// Halo CE Universal launcher: installs, updates and starts the native Windows
// build of https://github.com/cybersecurity/halo-ce-universal.
//
// The player supplies only what cannot be downloaded: the game data from their
// own copy of Halo: Combat Evolved for the Xbox (the PAL release, build
// 01.01.14.2342, or the NTSC release, build 01.10.12.2276, whose build it adds
// to the game's two map checks: SourcePatches). The Xbox SDK isn't needed:
// the native builds use the clean SDK declarations in port/include/xdk.
// Everything else comes from its official
// source and is checked: the game's source (GitHub), Visual Studio Build Tools
// 2022 (Microsoft, only when no Visual Studio has the x86 C++ libraries and a
// Windows SDK), clang (the Build Tools' own, an installed LLVM, or LLVM's
// GitHub release), Python (python.org's NuGet package) and ninja (GitHub); the
// game's configure.py fetches SDL3 itself.
//
// It is written for the C# 5 compiler that is part of Windows (.NET Framework
// 4), so building it needs nothing installed: see HaloLauncher.cmd and
// README.md next to this file.

using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

[assembly: AssemblyTitle("Halo CE Universal Launcher")]
[assembly: AssemblyProduct("Halo CE Universal Launcher")]
// SelfUpdate compares this with the newest release's launcher.txt
[assembly: AssemblyVersion("1.3.0.0")]
[assembly: AssemblyFileVersion("1.3.0.0")]

namespace HaloLauncher
{
	/* ---------- what is downloaded, and what the player's files must be */

	static class Pinned
	{
		public const string Repository = "cybersecurity/halo-ce-universal";
		public const string Branch = "main";
		public const string RepositoryUrl = "https://github.com/" + Repository;
		public const string SourceZipUrl = "https://codeload.github.com/" + Repository + "/zip/refs/heads/" + Branch;
		public const string CommitApiUrl = "https://api.github.com/repos/" + Repository + "/commits/" + Branch;

		// the launcher's own releases: each carries HaloLauncher.exe and
		// launcher.txt (SelfUpdate)
		public const string LauncherReleases = "https://github.com/bnunu/halo-ce-universal/releases";

		public const string PythonVersion = "3.13.15";
		public const string PythonUrl = "https://api.nuget.org/v3-flatcontainer/python/3.13.15/python.3.13.15.nupkg";
		public const string PythonSha256 = "05357887df50d3153efc681bdf432c321d3e2f9ce5788f99f4515b27e8fda0ac";

		public const string NinjaVersion = "1.13.2";
		public const string NinjaUrl = "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip";
		public const string NinjaSha256 = "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65";

		// Only downloaded when no clang of at least MinimumClangMajor (with
		// lld-link next to it) is installed; the game is known to build with 19
		// (Visual Studio 2022's) and 22.
		public const int MinimumClangMajor = 19;
		public const string LlvmVersion = "22.1.8";
		public const string LlvmFolder = "clang+llvm-22.1.8-x86_64-pc-windows-msvc";
		public const string LlvmUrl = "https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/clang%2Bllvm-22.1.8-x86_64-pc-windows-msvc.tar.xz";
		public const string LlvmSha256 = "d96c2cc1736f4eb7fa43cb9bbdf56d93551a9ae0a9aadb9c99c3c3b2b712a234";

		public const string BuildToolsUrl = "https://aka.ms/vs/17/release/vs_BuildTools.exe";
		public static readonly string[] BuildToolsComponents =
		{
			"Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
			"Microsoft.VisualStudio.Component.Windows11SDK.26100",
			"Microsoft.VisualStudio.Component.VC.Llvm.Clang",
		};
		// Free space they need on the Windows drive, where Microsoft's
		// installer puts them whatever the install folder (the Windows SDK
		// alone is 2.2 GB, the MSVC tools 2.5 GB), with room for its
		// downloads. Short of it, the installer fails half-way through with
		// 0x80070070 (disk full).
		public const long BuildToolsSpace = 10L << 30;
		public const string BuildToolsSize = "about 7 GB";

		// in the source: the clean Xbox SDK declarations the native builds use
		// in place of the SDK (from upstream d9b12fd4)
		public const string CleanSdkHeader = @"port\include\xdk\xtl.h";

		// source/cache/cache_files.c accepts maps of the PAL build only; for
		// maps of the NTSC build, the launcher adds that build to the game's
		// checks (SourcePatches)
		public const string PalBuild = "01.01.14.2342";
		public const string NtscBuild = "01.10.12.2276";
	}

	/* ---------- where everything goes */

	sealed class Folders
	{
		public readonly string Root;

		public Folders(string root)
		{
			Root = root;
		}

		public string Game { get { return Path.Combine(Root, "game"); } }
		public string Data { get { return Path.Combine(Root, "data"); } }
		public string Tools { get { return Path.Combine(Root, "tools"); } }
		public string Downloads { get { return Path.Combine(Root, "downloads"); } }
		public string Logs { get { return Path.Combine(Root, "logs"); } }
		public string PythonDirectory { get { return Path.Combine(Tools, "python"); } }
		public string PythonExe { get { return Path.Combine(PythonDirectory, "python.exe"); } }
		public string NinjaDirectory { get { return Path.Combine(Tools, "ninja"); } }
		public string NinjaExe { get { return Path.Combine(NinjaDirectory, "ninja.exe"); } }
		public string LlvmDirectory { get { return Path.Combine(Tools, "llvm"); } }
		public string HaloExe { get { return Path.Combine(Game, @"build\windows\halo.exe"); } }
		public string GameLog { get { return Path.Combine(Logs, "game.log"); } }
		public string LauncherLog { get { return Path.Combine(Logs, "launcher.log"); } }
		public string LauncherExe { get { return Path.Combine(Root, "HaloLauncher.exe"); } }
		public string IconFile { get { return Path.Combine(Root, "HaloLauncher.ico"); } }
		public string SettingsFile { get { return Path.Combine(Root, Settings.FileName); } }
		// the files the last source download installed, so the next one can
		// remove those that were deleted upstream
		public string SourceManifest { get { return Path.Combine(Game, ".launcher-files"); } }
		public bool GameBuilt { get { return File.Exists(HaloExe); } }

		// What is wrong with root as an install folder's path, or null; full
		// is the full path.
		public static string PathProblem(string root, out string full)
		{
			full = null;
			root = (root ?? "").Trim().Trim('"');
			if (root.Length == 0 || !Path.IsPathRooted(root) || root.StartsWith(@"\\"))
				return "Choose an install folder on this PC: a full path such as C:\\Games\\Halo CE Universal.";
			try
			{
				full = Path.GetFullPath(root).TrimEnd('\\');
			}
			catch (Exception error)
			{
				return "The install folder is not a valid path: " + error.Message;
			}
			if (full.Length <= 3)
				return "Choose a folder, not a whole drive.";
			if (full.IndexOfAny("%^&!\";|<>".ToCharArray()) >= 0)
				return "The install folder's name may not have any of % ^ & ! \" ; | < > in it (the build's command lines would break).";
			if (full.StartsWith(Util.ProgramFiles + "\\", StringComparison.OrdinalIgnoreCase) ||
				full.StartsWith(Util.ProgramFilesX86 + "\\", StringComparison.OrdinalIgnoreCase))
				return "Program Files needs administrator rights for every update; choose another folder.";
			return null;
		}

		// The launcher replaces and deletes folders inside its install folder,
		// so it only uses one of its own: a folder that does not exist yet, is
		// empty, holds only a built launcher and its log (written before the
		// settings), or has the launcher's settings.
		public static bool IsOwn(string root)
		{
			try
			{
				if (!Directory.Exists(root))
					return true;
				string settings = Path.Combine(root, Settings.FileName);
				if (File.Exists(settings))
					return Settings.IsLaunchers(settings);
				foreach (string entry in Directory.EnumerateFileSystemEntries(root))
				{
					string name = Path.GetFileName(entry);
					if (name.StartsWith("HaloLauncher.exe", StringComparison.OrdinalIgnoreCase) && File.Exists(entry))
						continue;
					if (name.Equals("logs", StringComparison.OrdinalIgnoreCase) && Directory.Exists(entry) &&
						Directory.EnumerateFileSystemEntries(entry).All(log =>
							Path.GetFileName(log).Equals("launcher.log", StringComparison.OrdinalIgnoreCase) && File.Exists(log)))
						continue;
					return false;
				}
				return true;
			}
			catch (UnauthorizedAccessException)
			{
				return false;
			}
			catch (IOException)
			{
				return false;
			}
		}

		// null when the launcher may install into root, else why not
		public static string Problem(string root, out string full)
		{
			string problem = PathProblem(root, out full);
			if (problem == null && !IsOwn(full))
				problem = full + " already has other files in it. The launcher installs only into a new or empty folder, or one it installed into before.";
			return problem;
		}
	}

	/* ---------- settings (launcher.ini in the install folder) */

	sealed class Settings
	{
		public const string FileName = "launcher.ini";
		const string Header = "# Halo CE Universal launcher settings";
		const string RegistryKey = @"Software\HaloCEUniversal";

		public string Root;
		public string Commit = "";
		public bool DeveloperBuild;
		public string DataInput = "";
		// the input the installed game data came from: choosing another one
		// replaces it
		public string InstalledData = "";
		public int WindowScale = 2;
		public decimal MouseSensitivity = 1m;
		public bool InvertMouse;
		public int VolumePercent = 100;
		public string Language = "";
		public bool Classic30Fps;
		public bool NoVsync;
		// "" finds an installed clang; "download" uses LLVM's release; else
		// the folder with clang.exe and lld-link.exe
		public string Clang = "";
		// whether the launcher installed Visual Studio Build Tools (uninstalling
		// mentions them)
		public bool InstalledBuildTools;
		public List<KeyValuePair<string, string>> Extra = new List<KeyValuePair<string, string>>();

		// The game opens its files with the Windows code page's names, so a
		// user folder with other letters in its name gets C:\Games instead.
		public static string DefaultRoot()
		{
			string local = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "HaloCEUniversal");
			if (local.All(c => c < 128))
				return local;
			return Path.Combine(Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows)), @"Games\HaloCEUniversal");
		}

		// whether path is this launcher's settings (and not another program's
		// file of the same name)
		public static bool IsLaunchers(string path)
		{
			try
			{
				using (StreamReader reader = new StreamReader(path, Encoding.UTF8))
					return reader.ReadLine() == Header;
			}
			catch (IOException)
			{
				return false;
			}
			catch (UnauthorizedAccessException)
			{
				return false;
			}
		}

		// An installed launcher finds the settings next to it; a fresh copy
		// finds the install the registry remembers, else starts a new one.
		public static Settings Load()
		{
			string root = null;
			string here = Path.GetDirectoryName(Application.ExecutablePath);
			if (IsLaunchers(Path.Combine(here, FileName)))
				root = here;
			if (root == null)
			{
				try
				{
					using (RegistryKey key = Registry.CurrentUser.OpenSubKey(RegistryKey))
					{
						string value = key == null ? null : key.GetValue("InstallRoot") as string;
						if (!string.IsNullOrEmpty(value) && IsLaunchers(Path.Combine(value, FileName)))
							root = value;
					}
				}
				catch (Exception)
				{
				}
			}
			return ForRoot(root ?? DefaultRoot());
		}

		public static Settings ForRoot(string root)
		{
			Settings settings = new Settings();
			settings.Root = root;
			string path = Path.Combine(root, FileName);
			if (IsLaunchers(path))
			{
				try
				{
					settings.Read(path);
				}
				catch (IOException)
				{
				}
				catch (UnauthorizedAccessException)
				{
				}
			}
			return settings;
		}

		public bool Exists { get { return IsLaunchers(Path.Combine(Root, FileName)); } }

		void Read(string path)
		{
			foreach (string line in File.ReadAllLines(path, Encoding.UTF8))
			{
				int equals = line.IndexOf('=');
				if (line.StartsWith("#") || equals <= 0)
					continue;
				string key = line.Substring(0, equals).Trim();
				string value = line.Substring(equals + 1).Trim();
				switch (key)
				{
				case "commit": Commit = value; break;
				case "developer_build": DeveloperBuild = value == "1"; break;
				case "data": DataInput = value; break;
				case "installed_data": InstalledData = value; break;
				case "window_scale": int.TryParse(value, out WindowScale); break;
				case "mouse_sensitivity": decimal.TryParse(value, NumberStyles.Number, CultureInfo.InvariantCulture, out MouseSensitivity); break;
				case "invert_mouse": InvertMouse = value == "1"; break;
				case "volume": int.TryParse(value, out VolumePercent); break;
				case "language": Language = value; break;
				case "classic_30fps": Classic30Fps = value == "1"; break;
				case "no_vsync": NoVsync = value == "1"; break;
				case "clang": Clang = value; break;
				case "installed_build_tools": InstalledBuildTools = value == "1"; break;
				default:
					if (key.StartsWith("env."))
						Extra.Add(new KeyValuePair<string, string>(key.Substring(4), value));
					break;
				}
			}
			WindowScale = Math.Max(1, Math.Min(4, WindowScale));
			if (MouseSensitivity <= 0)
				MouseSensitivity = 1m;
			VolumePercent = Math.Max(0, Math.Min(100, VolumePercent));
		}

		public void Save()
		{
			Directory.CreateDirectory(Root);
			var lines = new List<string>
			{
				Header,
				"commit=" + Commit,
				"developer_build=" + (DeveloperBuild ? "1" : "0"),
				"data=" + DataInput,
				"installed_data=" + InstalledData,
				"window_scale=" + WindowScale.ToString(CultureInfo.InvariantCulture),
				"mouse_sensitivity=" + MouseSensitivity.ToString(CultureInfo.InvariantCulture),
				"invert_mouse=" + (InvertMouse ? "1" : "0"),
				"volume=" + VolumePercent.ToString(CultureInfo.InvariantCulture),
				"language=" + Language,
				"classic_30fps=" + (Classic30Fps ? "1" : "0"),
				"no_vsync=" + (NoVsync ? "1" : "0"),
				"clang=" + Clang,
				"installed_build_tools=" + (InstalledBuildTools ? "1" : "0"),
			};
			foreach (KeyValuePair<string, string> pair in Extra)
				lines.Add("env." + pair.Key + "=" + pair.Value);
			File.WriteAllLines(Path.Combine(Root, FileName), lines, new UTF8Encoding(false));
			try
			{
				using (RegistryKey key = Registry.CurrentUser.CreateSubKey(RegistryKey))
					key.SetValue("InstallRoot", Root);
			}
			catch (Exception)
			{
			}
		}

		// The game's settings, as the HALO_* environment variables that take
		// precedence over its config.toml (port/linux/README.md).
		public void ApplyTo(System.Collections.Specialized.StringDictionary environment, Folders folders)
		{
			environment["HALO_DATA_ROOT"] = folders.Data;
			environment["HALO_WINDOW_SCALE"] = WindowScale.ToString(CultureInfo.InvariantCulture);
			if (MouseSensitivity != 1m)
				environment["HALO_MOUSE_SENSITIVITY"] = MouseSensitivity.ToString(CultureInfo.InvariantCulture);
			if (InvertMouse)
				environment["HALO_MOUSE_INVERT"] = "1";
			if (VolumePercent != 100)
				environment["HALO_VOLUME"] = (VolumePercent / 100.0).ToString("0.##", CultureInfo.InvariantCulture);
			if (Language.Length > 0)
				environment["HALO_LANGUAGE"] = Language;
			if (Classic30Fps)
				environment["HALO_INTERPOLATION"] = "0";
			if (NoVsync)
				environment["HALO_NO_VSYNC"] = "1";
			foreach (KeyValuePair<string, string> pair in Extra)
				environment[pair.Key] = pair.Value;
		}
	}

	/* ---------- reporting */

	// The installation's steps, which the setup window lists.
	enum InstallStep
	{
		CheckFiles,
		GetTools,
		GetSource,
		CopyFiles,
		Build,
	}

	// Where long steps report: the log, the status line and the progress bar.
	interface IReport
	{
		void Log(string line);
		void Status(string text);
		// total <= 0: unknown (then done counts bytes, if anything)
		void Progress(long done, long total);
		void Step(InstallStep step);
		bool IsCancelled { get; }
		// answered with OK (true) or Cancel
		bool Ask(string question);
	}

	// A problem the player can act on; its message is shown as it is.
	sealed class UserError : Exception
	{
		public UserError(string message) : base(message)
		{
		}
	}

	sealed class CancelledError : Exception
	{
		public CancelledError() : base("Cancelled.")
		{
		}
	}

	// Checks of the player's files write only to the log, and stop when
	// cancelled says so.
	sealed class LogOnlyReport : IReport
	{
		readonly IReport log;
		readonly Func<bool> cancelled;

		public LogOnlyReport(IReport log) : this(log, null)
		{
		}

		public LogOnlyReport(IReport log, Func<bool> cancelled)
		{
			this.log = log;
			this.cancelled = cancelled;
		}

		public void Log(string line) { if (log != null) log.Log(line); }
		public void Status(string text) { Log(text); }
		public void Progress(long done, long total) { }
		public void Step(InstallStep step) { }
		public bool IsCancelled { get { return cancelled != null && cancelled(); } }
		public bool Ask(string question) { return false; }
	}

	/* ---------- helpers */

	static class Util
	{
		// .NET Framework's default protocols do not include TLS 1.2, which
		// GitHub, NuGet and Microsoft require
		static Util()
		{
			ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;
		}

		public static string System32 { get { return Environment.GetFolderPath(Environment.SpecialFolder.System); } }

		public static string ProgramFiles
		{
			get
			{
				string native = Environment.GetEnvironmentVariable("ProgramW6432");
				return string.IsNullOrEmpty(native) ? Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles) : native;
			}
		}

		public static string ProgramFilesX86
		{
			get
			{
				string x86 = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
				return string.IsNullOrEmpty(x86) ? ProgramFiles : x86;
			}
		}

		public static void ThrowIfCancelled(IReport report)
		{
			if (report.IsCancelled)
				throw new CancelledError();
		}

		public static string Size(long bytes)
		{
			if (bytes >= 1L << 30)
				return (bytes / (double)(1L << 30)).ToString("0.0", CultureInfo.InvariantCulture) + " GB";
			if (bytes >= 1L << 20)
				return (bytes / (double)(1L << 20)).ToString("0", CultureInfo.InvariantCulture) + " MB";
			return (bytes / 1024.0).ToString("0", CultureInfo.InvariantCulture) + " KB";
		}

		// one argument, quoted for CommandLineToArgvW
		public static string Quote(string argument)
		{
			if (argument.Length > 0 && argument.IndexOfAny(new[] { ' ', '\t', '"' }) < 0)
				return argument;
			var quoted = new StringBuilder("\"");
			int backslashes = 0;
			foreach (char c in argument)
			{
				if (c == '\\')
				{
					backslashes++;
					continue;
				}
				quoted.Append('\\', c == '"' ? backslashes * 2 + 1 : backslashes);
				quoted.Append(c);
				backslashes = 0;
			}
			quoted.Append('\\', backslashes * 2);
			return quoted.Append('"').ToString();
		}

		public static string Sha256(string path)
		{
			using (SHA256 sha = SHA256.Create())
			using (FileStream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20))
				return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
		}

		public static int ReadUpTo(Stream stream, byte[] buffer, int count)
		{
			int total = 0;
			while (total < count)
			{
				int read = stream.Read(buffer, total, count - total);
				if (read <= 0)
					break;
				total += read;
			}
			return total;
		}

		public static byte[] ReadHead(string path, int count)
		{
			using (FileStream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
			{
				byte[] buffer = new byte[count];
				int read = ReadUpTo(stream, buffer, count);
				if (read < count)
					Array.Resize(ref buffer, read);
				return buffer;
			}
		}

		public static string NewDirectory(string parent, string name)
		{
			string path = Path.Combine(parent, name + "-" + Guid.NewGuid().ToString("N").Substring(0, 8));
			Directory.CreateDirectory(path);
			return path;
		}

		// what checks of the player's files left in %TEMP% when the launcher
		// was stopped during one
		public static void RemoveOldChecks()
		{
			try
			{
				foreach (string folder in Directory.GetDirectories(Path.GetTempPath(), "HaloCEUniversal-check-*"))
				{
					if (Directory.GetCreationTimeUtc(folder) < DateTime.UtcNow.AddHours(-1))
						TryDeleteDirectory(folder);
				}
			}
			catch (IOException)
			{
			}
			catch (UnauthorizedAccessException)
			{
			}
		}

		public static void TryDeleteDirectory(string path)
		{
			try
			{
				if (Directory.Exists(path))
					Directory.Delete(path, true);
			}
			catch (Exception)
			{
			}
		}

		public static void ReplaceDirectory(string staging, string target)
		{
			if (Directory.Exists(target))
				Directory.Delete(target, true);
			Directory.Move(staging, target);
		}

		public static void CopyDirectory(string source, string target)
		{
			Directory.CreateDirectory(target);
			foreach (string file in Directory.GetFiles(source))
				File.Copy(file, Path.Combine(target, Path.GetFileName(file)), true);
			foreach (string directory in Directory.GetDirectories(source))
				CopyDirectory(directory, Path.Combine(target, Path.GetFileName(directory)));
		}

		// Copies with progress into a .part file first, so an interrupted copy
		// is never taken for a complete one.
		public static void CopyStream(Stream input, string target, long length, IReport report, long before, long total)
		{
			string partial = target + ".part";
			byte[] buffer = new byte[1 << 20];
			long done = 0;
			using (FileStream output = new FileStream(partial, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
			{
				while (done < length)
				{
					int read = input.Read(buffer, 0, (int)Math.Min(buffer.Length, length - done));
					if (read <= 0)
						throw new IOException("unexpected end of " + Path.GetFileName(target));
					output.Write(buffer, 0, read);
					done += read;
					report.Progress(before + done, total);
					ThrowIfCancelled(report);
				}
			}
			if (File.Exists(target))
				File.Delete(target);
			File.Move(partial, target);
		}

		public static void CopyRange(string source, long offset, long length, string target, IReport report)
		{
			using (FileStream input = new FileStream(source, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20))
			{
				input.Position = offset;
				CopyStream(input, target, length, report, 0, length);
			}
		}

		// Files named like the pattern, at most depth folders down, looking at
		// no more than a few thousand folders (someone may pick a whole drive).
		public static List<string> FindFiles(string root, string pattern, int depth)
		{
			var found = new List<string>();
			var level = new List<string> { root };
			int visited = 0;
			for (int d = 0; d <= depth && level.Count > 0; d++)
			{
				var next = new List<string>();
				foreach (string directory in level)
				{
					if (++visited > 4000)
						return found;
					try
					{
						found.AddRange(Directory.GetFiles(directory, pattern));
						next.AddRange(Directory.GetDirectories(directory));
					}
					catch (UnauthorizedAccessException)
					{
					}
					catch (IOException)
					{
					}
				}
				level = next;
			}
			return found;
		}

		public static void RequireSpace(string path, long bytes, string purpose)
		{
			DriveInfo drive;
			try
			{
				drive = new DriveInfo(Path.GetPathRoot(Path.GetFullPath(path)));
				if (!drive.IsReady)
					return;
			}
			catch (ArgumentException)
			{
				return; // network paths
			}
			if (drive.AvailableFreeSpace < bytes)
			{
				throw new UserError(string.Format("Not enough free space on {0} for {1}: about {2} is needed and {3} is free. " +
					"Free up some space on {0} (empty the Recycle Bin, or delete or move files you don't need), then try again.",
					drive.Name.TrimEnd('\\'), purpose, Size(bytes), Size(drive.AvailableFreeSpace)));
			}
		}

		// the free space on the drive of path, or -1 if unknown
		public static long FreeSpace(string path)
		{
			try
			{
				var drive = new DriveInfo(Path.GetPathRoot(Path.GetFullPath(path)));
				return drive.IsReady ? drive.AvailableFreeSpace : -1;
			}
			catch (Exception)
			{
				return -1;
			}
		}

		public static bool SameDrive(string a, string b)
		{
			try
			{
				return string.Equals(Path.GetPathRoot(Path.GetFullPath(a)), Path.GetPathRoot(Path.GetFullPath(b)), StringComparison.OrdinalIgnoreCase);
			}
			catch (Exception)
			{
				return false;
			}
		}

		public static string Tail(string path, int lines)
		{
			try
			{
				using (FileStream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
				using (StreamReader reader = new StreamReader(stream))
				{
					string[] all = reader.ReadToEnd().Split(new[] { "\r\n", "\n" }, StringSplitOptions.None);
					return string.Join("\r\n", all.Skip(Math.Max(0, all.Length - lines)));
				}
			}
			catch (Exception)
			{
				return "";
			}
		}

		public static Dictionary<string, string> CurrentEnvironment()
		{
			var environment = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
			foreach (DictionaryEntry entry in Environment.GetEnvironmentVariables())
				environment[(string)entry.Key] = (string)entry.Value;
			return environment;
		}

		/* downloads */

		static HttpWebRequest Request(string url)
		{
			HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);
			request.UserAgent = "HaloCEUniversal-Launcher/1.0";
			request.Timeout = 30000;
			request.ReadWriteTimeout = 60000;
			return request;
		}

		public static string GetText(string url, string accept)
		{
			HttpWebRequest request = Request(url);
			if (accept != null)
				request.Accept = accept;
			using (WebResponse response = request.GetResponse())
			using (StreamReader reader = new StreamReader(response.GetResponseStream()))
				return reader.ReadToEnd();
		}

		// With sha256, a cached copy is reused and the download is verified.
		public static void Download(string url, string target, IReport report, string sha256)
		{
			if (sha256 != null && File.Exists(target) && Sha256(target) == sha256)
				return;
			Directory.CreateDirectory(Path.GetDirectoryName(target));
			string partial = target + ".part";
			string failure = null;
			for (int attempt = 1; attempt <= 3; attempt++)
			{
				try
				{
					DownloadOnce(url, partial, report);
					failure = null;
					break;
				}
				catch (WebException error)
				{
					failure = error.Message;
				}
				catch (IOException error)
				{
					failure = error.Message;
				}
				report.Log("Download failed (" + failure + ")" + (attempt < 3 ? ", retrying" : ""));
				if (attempt < 3)
					Thread.Sleep(2000 * attempt);
			}
			// Windows' curl.exe falls back from IPv6 to IPv4 at once, where .NET
			// keeps waiting on an IPv6 route that doesn't work until it times out
			if (failure != null && !CurlDownload(url, partial, report))
			{
				throw new UserError("Couldn't download " + url + " (" + failure + "). Check the internet connection, and that no " +
					"firewall or antivirus program blocks " + new Uri(url).Host + ", then try again.");
			}
			if (sha256 != null)
			{
				string actual = Sha256(partial);
				if (actual != sha256)
				{
					File.Delete(partial);
					throw new UserError("The file downloaded from " + url + " is not the expected one (SHA-256 " + actual + "), so it was not used. Try again later.");
				}
			}
			if (File.Exists(target))
				File.Delete(target);
			File.Move(partial, target);
		}

		// The same download with Windows' own curl.exe (Windows 10 1803 and
		// later); false when it is missing or fails too.
		static bool CurlDownload(string url, string path, IReport report)
		{
			string curl = Path.Combine(System32, "curl.exe");
			if (!File.Exists(curl))
				return false;
			report.Log("Trying again with Windows' curl.exe");
			report.Progress(0, 0);
			int code = Run(curl, "--fail --location --silent --show-error --retry 3 --retry-delay 2 --connect-timeout 30 " +
				"--speed-limit 1024 --speed-time 60 --output " + Quote(path) + " " + Quote(url), null, null, report);
			if (code != 0)
			{
				report.Log("curl.exe failed too (code " + code + ")");
				return false;
			}
			return File.Exists(path);
		}

		static void DownloadOnce(string url, string path, IReport report)
		{
			using (WebResponse response = Request(url).GetResponse())
			using (Stream input = response.GetResponseStream())
			using (FileStream output = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 16))
			{
				long total = response.ContentLength;
				long done = 0;
				byte[] buffer = new byte[1 << 16];
				int read;
				while ((read = input.Read(buffer, 0, buffer.Length)) > 0)
				{
					output.Write(buffer, 0, read);
					done += read;
					report.Progress(done, total);
					ThrowIfCancelled(report);
				}
				if (total > 0 && done != total)
					throw new IOException("the connection closed early");
			}
		}

		/* processes */

		// Runs a program, passing each line it prints to onLine; stops it (and
		// what it started) when the player cancels.
		public static int Run(string file, string arguments, string directory, Dictionary<string, string> environment,
			IReport report, Action<string> onLine, Encoding encoding)
		{
			ProcessStartInfo info = new ProcessStartInfo(file, arguments);
			info.UseShellExecute = false;
			info.CreateNoWindow = true;
			info.RedirectStandardOutput = true;
			info.RedirectStandardError = true;
			if (encoding != null)
			{
				info.StandardOutputEncoding = encoding;
				info.StandardErrorEncoding = encoding;
			}
			if (directory != null)
				info.WorkingDirectory = directory;
			if (environment != null)
			{
				info.EnvironmentVariables.Clear();
				foreach (KeyValuePair<string, string> pair in environment)
					info.EnvironmentVariables[pair.Key] = pair.Value;
			}
			using (Process process = new Process())
			{
				object sync = new object();
				DataReceivedEventHandler received = delegate(object sender, DataReceivedEventArgs e)
				{
					if (e.Data != null)
						lock (sync)
							onLine(e.Data);
				};
				process.StartInfo = info;
				process.OutputDataReceived += received;
				process.ErrorDataReceived += received;
				try
				{
					process.Start();
				}
				catch (Win32Exception error)
				{
					throw new UserError("Could not start " + file + ": " + error.Message);
				}
				process.BeginOutputReadLine();
				process.BeginErrorReadLine();
				while (!process.WaitForExit(250))
				{
					if (report.IsCancelled)
					{
						KillTree(process.Id);
						throw new CancelledError();
					}
				}
				process.WaitForExit(); // the rest of the output
				return process.ExitCode;
			}
		}

		public static int Run(string file, string arguments, string directory, Dictionary<string, string> environment, IReport report)
		{
			return Run(file, arguments, directory, environment, report, report.Log, null);
		}

		static void KillTree(int processId)
		{
			try
			{
				ProcessStartInfo info = new ProcessStartInfo(Path.Combine(System32, "taskkill.exe"), "/T /F /PID " + processId);
				info.UseShellExecute = false;
				info.CreateNoWindow = true;
				using (Process process = Process.Start(info))
					process.WaitForExit(10000);
			}
			catch (Exception)
			{
			}
		}

		// Starts a program with administrator rights (Windows asks the player).
		public static int RunElevated(string file, string arguments, IReport report)
		{
			ProcessStartInfo info = new ProcessStartInfo(file, arguments);
			info.UseShellExecute = true;
			info.Verb = "runas";
			Process process;
			try
			{
				process = Process.Start(info);
			}
			catch (Win32Exception error)
			{
				if (error.NativeErrorCode == 1223) // ERROR_CANCELLED
					throw new UserError("Windows' permission request was declined, so " + Path.GetFileName(file) + " didn't run. Try again, and click Yes when Windows asks.");
				throw new UserError("Could not start " + file + ": " + error.Message);
			}
			using (process)
			{
				while (!process.WaitForExit(500))
				{
					if (report.IsCancelled)
						throw new UserError("Stopped waiting for " + Path.GetFileName(file) + "; it keeps running in its own window.");
				}
				return process.ExitCode;
			}
		}
	}

	/* ---------- Windows' own tar.exe (libarchive): zip, 7z, rar, cab and ISO 9660 */

	static class Tar
	{
		static string Exe { get { return Path.Combine(Util.System32, "tar.exe"); } }

		static void RequireTar()
		{
			if (!File.Exists(Exe))
				throw new UserError("Windows' tar.exe was not found; it is part of Windows 10 (version 1803 and later) and Windows 11.");
		}

		// the archive's entries with '/' separators, or null if tar cannot read it
		public static List<string> List(string archive, IReport report)
		{
			RequireTar();
			var entries = new List<string>();
			int code = Util.Run(Exe, "-tf " + Util.Quote(archive), null, null, report,
				delegate(string line) { entries.Add(line.Replace('\\', '/')); }, null);
			return code == 0 ? entries : null;
		}

		public static void Extract(string archive, string target, int stripComponents, IEnumerable<string> members, IReport report)
		{
			RequireTar();
			Directory.CreateDirectory(target);
			var arguments = new StringBuilder("-xf " + Util.Quote(archive) + " -C " + Util.Quote(target));
			if (stripComponents > 0)
				arguments.Append(" --strip-components=" + stripComponents);
			foreach (string member in members)
				arguments.Append(' ').Append(Util.Quote(member));
			var output = new List<string>();
			int code = Util.Run(Exe, arguments.ToString(), null, null, report, output.Add, null);
			if (code != 0)
			{
				throw new UserError("Couldn't unpack " + Path.GetFileName(archive) + ": " +
					string.Join(" ", output.Take(4)));
			}
		}
	}

	/* ---------- the game data */

	// An Xbox disc image (XDVDFS): an extract-xiso image, or a full dump whose
	// game partition starts further in (XGD1, 2 or 3).
	sealed class XboxImage : IDisposable
	{
		const string Magic = "MICROSOFT*XBOX*MEDIA";
		const int Sector = 2048;
		static readonly long[] PartitionOffsets = { 0, 0x18300000L, 0x0FD90000L, 0x02080000L };

		public sealed class Entry
		{
			public string Name;
			public uint Start;
			public uint Size;
			public bool IsDirectory;
		}

		readonly FileStream stream;
		readonly long partition;
		readonly uint rootStart, rootSize;

		XboxImage(FileStream stream, long partition, uint rootStart, uint rootSize)
		{
			this.stream = stream;
			this.partition = partition;
			this.rootStart = rootStart;
			this.rootSize = rootSize;
		}

		public static XboxImage TryOpen(string path)
		{
			FileStream stream = null;
			try
			{
				stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 16);
				byte[] descriptor = new byte[Sector];
				foreach (long offset in PartitionOffsets)
				{
					if (offset + 33L * Sector > stream.Length)
						continue;
					stream.Position = offset + 32L * Sector;
					if (Util.ReadUpTo(stream, descriptor, Sector) < Sector)
						continue;
					if (Encoding.ASCII.GetString(descriptor, 0, 20) != Magic ||
						Encoding.ASCII.GetString(descriptor, 0x7EC, 20) != Magic)
						continue;
					XboxImage image = new XboxImage(stream, offset,
						BitConverter.ToUInt32(descriptor, 0x14), BitConverter.ToUInt32(descriptor, 0x18));
					stream = null;
					return image;
				}
				return null;
			}
			catch (IOException)
			{
				return null;
			}
			catch (UnauthorizedAccessException)
			{
				return null;
			}
			finally
			{
				if (stream != null)
					stream.Dispose();
			}
		}

		public List<Entry> Root()
		{
			return Directory(rootStart, rootSize);
		}

		public List<Entry> Directory(Entry directory)
		{
			return Directory(directory.Start, directory.Size);
		}

		// A directory is a binary tree of entries; offsets count 4-byte words.
		List<Entry> Directory(uint start, uint size)
		{
			var entries = new List<Entry>();
			if (size == 0 || size > 16 << 20)
				return entries;
			byte[] table = new byte[size];
			stream.Position = partition + (long)start * Sector;
			int length = Util.ReadUpTo(stream, table, table.Length);
			var pending = new Stack<int>();
			var seen = new HashSet<int>();
			pending.Push(0);
			while (pending.Count > 0)
			{
				int offset = pending.Pop();
				if (offset + 14 > length || !seen.Add(offset))
					continue;
				int left = BitConverter.ToUInt16(table, offset);
				int right = BitConverter.ToUInt16(table, offset + 2);
				if (left == 0xFFFF && right == 0xFFFF)
					continue;
				int nameLength = table[offset + 13];
				if (offset + 14 + nameLength > length)
					continue;
				entries.Add(new Entry
				{
					Start = BitConverter.ToUInt32(table, offset + 4),
					Size = BitConverter.ToUInt32(table, offset + 8),
					IsDirectory = (table[offset + 12] & 0x10) != 0,
					Name = Encoding.ASCII.GetString(table, offset + 14, nameLength),
				});
				if (left != 0)
					pending.Push(left * 4);
				if (right != 0)
					pending.Push(right * 4);
			}
			return entries.OrderBy(e => e.Name, StringComparer.OrdinalIgnoreCase).ToList();
		}

		public byte[] ReadHead(Entry file, int count)
		{
			byte[] buffer = new byte[Math.Min(count, (int)Math.Min(file.Size, int.MaxValue))];
			stream.Position = partition + (long)file.Start * Sector;
			Util.ReadUpTo(stream, buffer, buffer.Length);
			return buffer;
		}

		public void Extract(Entry file, string target, IReport report, long before, long total)
		{
			stream.Position = partition + (long)file.Start * Sector;
			Util.CopyStream(stream, target, file.Size, report, before, total);
		}

		public void Dispose()
		{
			stream.Dispose();
		}
	}

	// One cache file, identified by its header (struct cache_file_header in
	// source/cache/cache_files_windows.c).
	sealed class MapFile
	{
		public string Folder;  // maps, maps_de, ...
		public string Name;    // a10.map
		public long Size;
		public bool Valid;
		public string Build;
		public byte[] Header;
		public string SourcePath;
		public XboxImage.Entry ImageEntry;

		public const int HeaderSize = 0x800;

		public static MapFile Parse(string folder, string name, long size, byte[] header)
		{
			MapFile map = new MapFile();
			map.Folder = folder;
			map.Name = name;
			map.Size = size;
			map.Header = header;
			map.Build = "";
			if (header.Length >= HeaderSize &&
				Encoding.ASCII.GetString(header, 0, 4) == "daeh" && Encoding.ASCII.GetString(header, 0x7FC, 4) == "toof" &&
				BitConverter.ToInt32(header, 4) == 5)
			{
				map.Valid = true;
				map.Build = Encoding.ASCII.GetString(header, 0x40, 32).Split('\0')[0];
			}
			return map;
		}
	}

	abstract class GameDataSource : IDisposable
	{
		public string Description;
		public string Location = ""; // the folder or disc image the maps are in
		public readonly List<MapFile> Maps = new List<MapFile>();

		public string Friendly()
		{
			return "This is Halo: Combat Evolved, " + GameData.VersionName(Build) + ": " + Maps.Count + " maps (" + Util.Size(TotalBytes) + ").";
		}

		// the maps' build, once GameData.Problem found them all the same
		public string Build { get { return Maps.Count > 0 ? Maps[0].Build : ""; } }

		public long TotalBytes { get { return Maps.Sum(m => m.Size); } }

		// with replace, every map is copied again
		public long BytesToCopy(string dataRoot, bool replace)
		{
			return Maps.Where(m => replace || !AlreadyThere(dataRoot, m)).Sum(m => m.Size);
		}

		// the same map: same size and the same header, which has its name,
		// build and checksum (an interrupted copy never has the final name)
		static bool AlreadyThere(string dataRoot, MapFile map)
		{
			string path = Path.Combine(dataRoot, map.Folder, map.Name);
			FileInfo existing = new FileInfo(path);
			if (!existing.Exists || existing.Length != map.Size)
				return false;
			try
			{
				return Util.ReadHead(path, MapFile.HeaderSize).SequenceEqual(map.Header);
			}
			catch (IOException)
			{
				return false;
			}
		}

		public string Summary()
		{
			var folders = Maps.Select(m => m.Folder).Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(f => f);
			return string.Format("{0} maps, build {1} ({2}; {3}) from {4}.", Maps.Count, Build,
				string.Join(", ", folders), Util.Size(TotalBytes), Description);
		}

		// Copies the maps folders into dataRoot, replacing maps folders that
		// this game data does not have; with replace, every map is copied.
		public void CopyTo(string dataRoot, IReport report, bool replace)
		{
			Directory.CreateDirectory(dataRoot);
			var folders = new HashSet<string>(Maps.Select(m => m.Folder), StringComparer.OrdinalIgnoreCase);
			foreach (string existing in Directory.GetDirectories(dataRoot))
			{
				if (GameData.IsMapsFolder(Path.GetFileName(existing)) && !folders.Contains(Path.GetFileName(existing)))
					Directory.Delete(existing, true);
			}
			long total = BytesToCopy(dataRoot, replace), done = 0;
			foreach (MapFile map in Maps)
			{
				string target = Path.Combine(dataRoot, map.Folder, map.Name);
				if (!replace && AlreadyThere(dataRoot, map))
					continue;
				Directory.CreateDirectory(Path.GetDirectoryName(target));
				report.Log("  " + map.Folder + "\\" + map.Name + " (" + Util.Size(map.Size) + ")");
				CopyMap(map, target, report, done, total);
				done += map.Size;
			}
			foreach (string folder in folders)
			{
				var names = new HashSet<string>(Maps.Where(m => m.Folder.Equals(folder, StringComparison.OrdinalIgnoreCase)).Select(m => m.Name), StringComparer.OrdinalIgnoreCase);
				foreach (string file in Directory.GetFiles(Path.Combine(dataRoot, folder), "*.map"))
				{
					if (!names.Contains(Path.GetFileName(file)))
						File.Delete(file);
				}
			}
		}

		protected abstract void CopyMap(MapFile map, string target, IReport report, long before, long total);

		public virtual void Dispose()
		{
		}
	}

	sealed class FolderSource : GameDataSource
	{
		public FolderSource(string root)
		{
			Description = root;
			Location = root;
			foreach (string folder in System.IO.Directory.GetDirectories(root).Where(d => GameData.IsMapsFolder(Path.GetFileName(d))))
			{
				foreach (string file in System.IO.Directory.GetFiles(folder, "*.map"))
				{
					MapFile map = MapFile.Parse(Path.GetFileName(folder), Path.GetFileName(file), new FileInfo(file).Length,
						Util.ReadHead(file, MapFile.HeaderSize));
					map.SourcePath = file;
					Maps.Add(map);
				}
			}
		}

		protected override void CopyMap(MapFile map, string target, IReport report, long before, long total)
		{
			if (string.Equals(Path.GetFullPath(map.SourcePath), Path.GetFullPath(target), StringComparison.OrdinalIgnoreCase))
				return;
			using (FileStream input = new FileStream(map.SourcePath, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20))
				Util.CopyStream(input, target, map.Size, report, before, total);
		}
	}

	sealed class ImageSource : GameDataSource
	{
		readonly XboxImage image;

		public ImageSource(XboxImage image, string path)
		{
			this.image = image;
			Description = path;
			Location = path;
			foreach (XboxImage.Entry folder in image.Root().Where(e => e.IsDirectory && GameData.IsMapsFolder(e.Name)))
			{
				// a name is a file name: nothing in it may lead out of the maps folder
				foreach (XboxImage.Entry file in image.Directory(folder).Where(e => !e.IsDirectory &&
					e.Name.EndsWith(".map", StringComparison.OrdinalIgnoreCase) && e.Name.IndexOfAny(Path.GetInvalidFileNameChars()) < 0))
				{
					MapFile map = MapFile.Parse(folder.Name, file.Name, file.Size, image.ReadHead(file, MapFile.HeaderSize));
					map.ImageEntry = file;
					Maps.Add(map);
				}
			}
		}

		protected override void CopyMap(MapFile map, string target, IReport report, long before, long total)
		{
			image.Extract(map.ImageEntry, target, report, before, total);
		}

		public override void Dispose()
		{
			image.Dispose();
		}
	}

	// Only while checking: an archive is unpacked during the installation.
	sealed class ArchiveSource : GameDataSource
	{
		public ArchiveSource(string description)
		{
			Description = description;
		}

		protected override void CopyMap(MapFile map, string target, IReport report, long before, long total)
		{
			throw new InvalidOperationException();
		}
	}

	static class GameData
	{
		static readonly Regex MapsFolder = new Regex(@"^maps(_[a-z]{2})?$", RegexOptions.IgnoreCase);
		// an archive's map file: 1 is the folders above maps, 2 the maps folder
		static readonly Regex MapEntry = new Regex(@"^(.*/)?(maps(_[a-z]{2})?)/[^/]+\.map$", RegexOptions.IgnoreCase);
		static readonly Regex ImageEntry = new Regex(@"\.(iso|xiso)$", RegexOptions.IgnoreCase);

		public static bool IsMapsFolder(string name)
		{
			return MapsFolder.IsMatch(name);
		}

		public static bool IsInstalled(string dataRoot)
		{
			return InstalledBuild(dataRoot) != null;
		}

		// the build of the maps installed in dataRoot, or null if they are
		// missing or unusable
		public static string InstalledBuild(string dataRoot)
		{
			if (!Directory.Exists(dataRoot))
				return null;
			using (FolderSource installed = new FolderSource(dataRoot))
				return Problem(installed.Maps) == null ? installed.Build : null;
		}

		public static string VersionName(string build)
		{
			if (build == Pinned.PalBuild)
				return "European version";
			if (build == Pinned.NtscBuild)
				return "American version";
			return "build " + build;
		}

		// What is wrong with these maps for this game, or null.
		public static string Problem(List<MapFile> maps)
		{
			if (maps.Count == 0)
				return "No Halo game files were found here. Choose the disc image (.iso), or the folder that has a folder called maps in it.";
			MapFile invalid = maps.FirstOrDefault(m => !m.Valid);
			if (invalid != null)
				return invalid.Folder + "\\" + invalid.Name + " is damaged or isn't a Halo file. Copy the game from the disc again.";
			MapFile other = maps.FirstOrDefault(m => m.Build != Pinned.PalBuild && m.Build != Pinned.NtscBuild);
			if (other != null)
			{
				return "This is a different version of Halo (build " + other.Build + "). This PC version works with the American (NTSC) " +
					"and the European (PAL) versions of the game, builds " + Pinned.NtscBuild + " and " + Pinned.PalBuild + ".";
			}
			if (maps.Any(m => m.Build != maps[0].Build))
				return "These game files mix the American and the European version of Halo. Copy the maps folder from one disc only.";
			if (!maps.Any(m => m.Name.Equals("ui.map", StringComparison.OrdinalIgnoreCase)))
				return "ui.map (the main menu) is missing. Copy the whole maps folder from the disc.";
			return null;
		}

		// A folder holding the game's files, a disc image, or an archive
		// holding either. With unpack false (a quick check), an archive is only
		// listed, since unpacking it can take minutes.
		public static GameDataSource Open(string input, string scratch, IReport report, bool unpack)
		{
			return Open(input, scratch, report, unpack, 0);
		}

		static GameDataSource Open(string input, string scratch, IReport report, bool unpack, int depth)
		{
			Util.ThrowIfCancelled(report);
			if (depth > 2)
				return null;
			if (Directory.Exists(input))
				return Best(Candidates(input));
			if (!File.Exists(input))
				return null;
			if (input.EndsWith(".map", StringComparison.OrdinalIgnoreCase))
				return Open(Path.GetDirectoryName(input), scratch, report, unpack, depth + 1);
			GameDataSource source = OpenImage(input);
			if (source != null)
				return source;
			if (!IsArchive(input) && !ImageEntry.IsMatch(input))
				return null;
			report.Log("Looking into " + input);
			List<string> entries = Tar.List(input, report);
			if (entries == null)
				return null;
			var maps = entries.Select(e => MapEntry.Match(e)).Where(m => m.Success).ToList();
			if (maps.Count > 0)
			{
				// the maps folders next to the first one found
				string prefix = maps[0].Groups[1].Value;
				maps = maps.Where(m => m.Groups[1].Value == prefix).ToList();
				if (!unpack)
					return new ArchiveSource("Found Halo's game files inside " + Path.GetFileName(input) + ". They are checked while installing.");
				var folders = maps.Select(m => prefix + m.Groups[2].Value).Distinct(StringComparer.OrdinalIgnoreCase).ToList();
				string target = Util.NewDirectory(scratch, "data");
				report.Status("Unpacking your game files from " + Path.GetFileName(input));
				Tar.Extract(input, target, prefix.Count(c => c == '/'), folders, report);
				return new FolderSource(target);
			}
			string nested = entries.FirstOrDefault(e => ImageEntry.IsMatch(e));
			if (nested != null)
			{
				if (!unpack)
					return new ArchiveSource("Found a disc image inside " + Path.GetFileName(input) + ". It is checked while installing.");
				string target = Util.NewDirectory(scratch, "image");
				report.Status("Unpacking the disc image from " + Path.GetFileName(input));
				Tar.Extract(input, target, 0, new[] { nested }, report);
				return Open(Path.Combine(target, nested.Replace('/', '\\')), scratch, report, unpack, depth + 1);
			}
			return null;
		}

		static bool IsArchive(string path)
		{
			string extension = Path.GetExtension(path).ToLowerInvariant();
			return extension == ".zip" || extension == ".7z" || extension == ".rar";
		}

		// the disc image's maps, or null if it is not an Xbox disc image
		static GameDataSource OpenImage(string path)
		{
			XboxImage image = XboxImage.TryOpen(path);
			if (image == null)
				return null;
			try
			{
				return new ImageSource(image, path);
			}
			catch (Exception)
			{
				image.Dispose();
				throw;
			}
		}

		// 2: this game's data; 1: Halo maps of another build; 0: other files
		static int Score(GameDataSource source)
		{
			if (Problem(source.Maps) == null)
				return 2;
			return source.Maps.Count > 0 && source.Maps.All(m => m.Valid) ? 1 : 0;
		}

		// Of the candidates, the first that works; else the most Halo-like
		// one, so that its problem is the one reported (someone may choose
		// their whole Downloads folder, with other games' maps in it).
		static GameDataSource Best(IEnumerable<Func<GameDataSource>> candidates)
		{
			GameDataSource best = null;
			foreach (Func<GameDataSource> candidate in candidates)
			{
				GameDataSource source;
				try
				{
					source = candidate();
				}
				catch (IOException)
				{
					continue;
				}
				catch (UnauthorizedAccessException)
				{
					continue;
				}
				if (source == null)
					continue;
				if (Score(source) == 2)
				{
					if (best != null)
						best.Dispose();
					return source;
				}
				if (source.Maps.Count > 0 && (best == null || Score(source) > Score(best)))
				{
					if (best != null)
						best.Dispose();
					best = source;
				}
				else
				{
					source.Dispose();
				}
			}
			return best;
		}

		// In a folder: the folders holding maps (the one chosen, its parent
		// when a maps folder was chosen, and those below), then disc images
		// (not archives, which could hold anything and take minutes to unpack).
		static IEnumerable<Func<GameDataSource>> Candidates(string folder)
		{
			foreach (string root in DataRoots(folder))
			{
				string found = root;
				yield return delegate { return new FolderSource(found); };
			}
			foreach (string file in Util.FindFiles(folder, "*", 2).Where(f => ImageEntry.IsMatch(f)).Take(8))
			{
				string image = file;
				yield return delegate { return OpenImage(image); };
			}
		}

		static List<string> DataRoots(string folder)
		{
			var roots = new List<string>();
			string parent = Path.GetDirectoryName(folder.TrimEnd('\\'));
			if (IsMapsFolder(Path.GetFileName(folder.TrimEnd('\\'))) && parent != null && HasMaps(parent))
				roots.Add(parent);
			var level = new List<string> { folder };
			int visited = 0;
			for (int depth = 0; depth <= 3 && roots.Count < 16; depth++)
			{
				var next = new List<string>();
				foreach (string directory in level)
				{
					if (++visited > 2000)
						return roots;
					if (HasMaps(directory))
						roots.Add(directory);
					try
					{
						next.AddRange(Directory.GetDirectories(directory));
					}
					catch (UnauthorizedAccessException)
					{
					}
					catch (IOException)
					{
					}
				}
				level = next;
			}
			return roots;
		}

		static bool HasMaps(string folder)
		{
			try
			{
				return Directory.GetDirectories(folder).Any(d => IsMapsFolder(Path.GetFileName(d)) && Directory.GetFiles(d, "*.map").Length > 0);
			}
			catch (UnauthorizedAccessException)
			{
				return false;
			}
			catch (IOException)
			{
				return false;
			}
		}
	}

	/* ---------- compilers and build tools */

	sealed class Toolchain
	{
		public string VisualStudio;
		public Dictionary<string, string> Environment; // vcvarsall x86
		public string ClangDirectory;
		public bool InstalledBuildTools; // by this run

		// Whether a Visual Studio has the x86 C++ libraries (a quick look,
		// for telling the player in advance; Prepare checks properly).
		public static bool HasBuildTools()
		{
			try
			{
				return Instances("-products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64", new LogOnlyReport(null))
					.Any(HasX86Libraries);
			}
			catch (Exception)
			{
				return false;
			}
		}

		public static Toolchain Prepare(Folders folders, string clang, IReport report)
		{
			Toolchain tools = new Toolchain();
			report.Status("Looking for Microsoft's C++ build tools");
			string missing = tools.FindVisualStudio(report);
			if (missing != null)
			{
				report.Log(missing);
				InstallBuildTools(folders, report);
				tools.InstalledBuildTools = true;
				missing = tools.FindVisualStudio(report);
				if (missing != null)
					throw new UserError("Microsoft's C++ build tools were installed, but Halo still can't use them: " + missing + " Restart Windows, then try again.");
			}
			report.Log("Visual Studio: " + tools.VisualStudio);

			report.Status("Looking for the clang compiler");
			if (clang == "download")
				tools.ClangDirectory = InstallLlvm(folders, report);
			else if (clang.Length > 0)
				tools.ClangDirectory = UsableClang(clang, report) ? clang : null;
			else
				tools.ClangDirectory = FindClang(folders, tools.VisualStudio, report) ?? InstallLlvm(folders, report);
			if (tools.ClangDirectory == null)
				throw new UserError("launcher.ini names " + clang + " for clang, but it has no clang.exe " + Pinned.MinimumClangMajor + " or later with lld-link.exe.");
			report.Log("clang: " + tools.ClangDirectory);

			InstallPython(folders, report);
			InstallNinja(folders, report);
			return tools;
		}

		// the build's environment: vcvarsall x86, with clang, Python and ninja first on PATH
		public Dictionary<string, string> BuildEnvironment(Folders folders)
		{
			var environment = new Dictionary<string, string>(Environment ?? Util.CurrentEnvironment(), StringComparer.OrdinalIgnoreCase);
			string path;
			environment.TryGetValue("PATH", out path);
			environment["PATH"] = string.Join(";", new[] { ClangDirectory, folders.PythonDirectory, folders.NinjaDirectory, path ?? "" });
			environment["PYTHONUTF8"] = "1";
			environment["PYTHONDONTWRITEBYTECODE"] = "1";
			return environment;
		}

		static string Vswhere { get { return Path.Combine(Util.ProgramFilesX86, @"Microsoft Visual Studio\Installer\vswhere.exe"); } }

		static List<string> Instances(string arguments, IReport report)
		{
			var paths = new List<string>();
			if (!File.Exists(Vswhere))
				return paths;
			Util.Run(Vswhere, arguments + " -utf8 -format value -property installationPath", null, null, report,
				delegate(string line) { if (line.Trim().Length > 0) paths.Add(line.Trim()); }, Encoding.UTF8);
			return paths;
		}

		// Finds an instance with the x86 C++ libraries and a Windows SDK, and
		// keeps its x86 environment. Returns what is missing, or null.
		string FindVisualStudio(IReport report)
		{
			List<string> instances = Instances("-products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64", report);
			if (instances.Count == 0)
				return "No Visual Studio with the C++ build tools is installed.";
			string missing = "No Visual Studio has the x86 C++ libraries.";
			foreach (string instance in instances)
			{
				if (!HasX86Libraries(instance))
					continue;
				Dictionary<string, string> environment = CaptureEnvironment(instance, report);
				if (environment == null)
				{
					missing = "vcvarsall.bat failed in " + instance + ".";
					continue;
				}
				string library = MissingLibrary(environment);
				if (library != null)
				{
					missing = "The Windows SDK is missing (" + library + " was not found).";
					continue;
				}
				VisualStudio = instance;
				Environment = environment;
				return null;
			}
			return missing;
		}

		static bool HasX86Libraries(string instance)
		{
			string versionFile = Path.Combine(instance, @"VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt");
			if (!File.Exists(versionFile))
				return false;
			string version = File.ReadAllText(versionFile).Trim();
			return File.Exists(Path.Combine(instance, @"VC\Tools\MSVC", version, @"lib\x86\libcmt.lib"));
		}

		// runs vcvarsall.bat x86 and reads back the environment it set
		// (cmd /u makes set write UTF-16, so no character is lost)
		static Dictionary<string, string> CaptureEnvironment(string instance, IReport report)
		{
			string vcvars = Path.Combine(instance, @"VC\Auxiliary\Build\vcvarsall.bat");
			if (!File.Exists(vcvars))
				return null;
			Dictionary<string, string> environment = Util.CurrentEnvironment();
			string path;
			environment.TryGetValue("PATH", out path);
			environment["PATH"] = Path.GetDirectoryName(Vswhere) + ";" + path;
			var captured = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
			int code = Util.Run(Path.Combine(Util.System32, "cmd.exe"), "/d /u /s /c \"\"" + vcvars + "\" x86 >nul 2>&1 && set\"",
				null, environment, report,
				delegate(string line)
				{
					int equals = line.IndexOf('=');
					if (equals > 0)
						captured[line.Substring(0, equals)] = line.Substring(equals + 1);
				},
				Encoding.Unicode);
			return code == 0 && captured.ContainsKey("LIB") ? captured : null;
		}

		static string MissingLibrary(Dictionary<string, string> environment)
		{
			string[] directories = environment["LIB"].Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries);
			foreach (string library in new[] { "libcmt.lib", "libucrt.lib", "kernel32.lib" })
			{
				if (!directories.Any(d => File.Exists(Path.Combine(d.Trim(), library))))
					return library;
			}
			return null;
		}

		static void InstallBuildTools(Folders folders, IReport report)
		{
			if (!report.Ask("Halo is built on your PC with Microsoft's free C++ build tools (Visual Studio Build Tools " +
				"2022, " + Pinned.BuildToolsSize + " on " + Path.GetPathRoot(Util.ProgramFilesX86).TrimEnd('\\') + "), and this PC doesn't have them yet.\r\n\r\n" +
				"Click OK to install them. Windows will ask for permission: click Yes. Microsoft's installer then shows " +
				"its progress; it usually takes 10 to 30 minutes."))
			{
				throw new UserError("Halo can't be built without Microsoft's C++ build tools. Try again and click OK to install " +
					"them, or install Visual Studio with \"Desktop development with C++\" yourself.");
			}
			string bootstrapper = Path.Combine(folders.Downloads, "vs_BuildTools.exe");
			report.Status("Downloading the installer of Microsoft's C++ build tools");
			Util.Download(Pinned.BuildToolsUrl, bootstrapper, report, null);

			var arguments = new StringBuilder();
			string existing = Instances("-products Microsoft.VisualStudio.Product.BuildTools -version [17.0,18.0)", report).FirstOrDefault();
			if (existing != null)
				arguments.Append("modify --installPath ").Append(Util.Quote(existing)).Append(' ');
			arguments.Append("--passive --wait --norestart --nocache");
			foreach (string component in Pinned.BuildToolsComponents)
				arguments.Append(" --add ").Append(component);

			report.Status("Installing Microsoft's C++ build tools (this usually takes 10 to 30 minutes)");
			LogFreeSpace(report);
			DateTime started = DateTime.UtcNow;
			int code = Util.RunElevated(bootstrapper, arguments.ToString(), report);
			if (code == 3010 || code == 1641)
				report.Log("The Visual Studio Installer asks for a restart of Windows; continuing without one.");
			else if (code != 0)
			{
				LogFreeSpace(report);
				LogInstallerErrors(report, started);
				throw new UserError(BuildToolsFailure(code));
			}
			File.Delete(bootstrapper);
		}

		static void LogFreeSpace(IReport report)
		{
			var drives = new List<string>();
			foreach (DriveInfo drive in DriveInfo.GetDrives())
			{
				try
				{
					if (drive.DriveType == DriveType.Fixed && drive.IsReady)
						drives.Add(drive.Name.TrimEnd('\\') + " " + Util.Size(drive.AvailableFreeSpace) + " of " + Util.Size(drive.TotalSize));
				}
				catch (IOException)
				{
				}
			}
			report.Log("Free space: " + string.Join(", ", drives) + ".");
		}

		// Microsoft's installer keeps its logs in %TEMP% (dd_*.log): the lines
		// of this run's logs that report errors go into the launcher's log,
		// so that "Copy details" says what failed, and where.
		static void LogInstallerErrors(IReport report, DateTime since)
		{
			try
			{
				var logs = new DirectoryInfo(Path.GetTempPath()).GetFiles("dd_*.log")
					.Where(f => f.LastWriteTimeUtc >= since.AddMinutes(-1)).OrderBy(f => f.LastWriteTimeUtc).ToList();
				foreach (FileInfo log in logs)
				{
					var errors = new Queue<string>();
					using (var reader = new StreamReader(new FileStream(log.FullName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete)))
					{
						string line;
						while ((line = reader.ReadLine()) != null)
						{
							if (line.IndexOf("error", StringComparison.OrdinalIgnoreCase) < 0 && line.IndexOf("0x8007", StringComparison.OrdinalIgnoreCase) < 0)
								continue;
							errors.Enqueue(line.Length > 400 ? line.Substring(0, 400) + "..." : line);
							if (errors.Count > 8)
								errors.Dequeue();
						}
					}
					foreach (string error in errors)
						report.Log(log.Name + ": " + error.Trim());
				}
			}
			catch (Exception error)
			{
				report.Log("Could not read the Visual Studio Installer's logs: " + error.Message);
			}
		}

		// The Visual Studio Installer's exit codes (its documentation's "Use
		// command-line parameters to install Visual Studio"), and the Windows
		// errors it passes on, in words.
		static string BuildToolsFailure(int code)
		{
			string drive = Path.GetPathRoot(Util.ProgramFilesX86).TrimEnd('\\');
			const string Failed = "Installing Microsoft's C++ build tools didn't finish: ";
			switch (code)
			{
				case -2147024784: // 0x80070070, ERROR_DISK_FULL
				case -2147024857: // 0x80070027, ERROR_HANDLE_DISK_FULL
					return Failed + drive + " is full. They need about " + Util.Size(Pinned.BuildToolsSpace) + " free on " + drive +
						", and " + Util.Size(Math.Max(0, Util.FreeSpace(Util.ProgramFilesX86))) + " is free now. Free up some space on " + drive +
						" (empty the Recycle Bin, or delete or move files you don't need), then try again.";
				case 1602:
				case 5004:
				case -1073741510: // 0xC000013A: its window was closed
					return Failed + "Microsoft's installer was closed before it was done. Try again, and leave its window open until it closes by itself.";
				case 1001:
				case 1618:
					return Failed + "another installation is running (Windows Update, or another Visual Studio Installer). Wait until it's done, or restart Windows, then try again.";
				case 1003:
				case 8006:
					return Failed + "Visual Studio or its installer is open. Close them, then try again.";
				case 5003:
				case -1073720687:
					return Failed + "Microsoft's installer couldn't download them. Check your internet connection, then try again.";
				case 740:
					return Failed + "it needs administrator permission. Try again, and click Yes when Windows asks.";
				case 5007:
				case 8010:
					return Failed + "Microsoft's installer says this PC doesn't meet the requirements of Visual Studio Build Tools 2022 " +
						"(64-bit Windows 10 version 1909 or later, or Windows 11).";
				default:
					return Failed + "Microsoft's installer stopped with code " + code + " (0x" + code.ToString("X8") + "). Try again; if it keeps " +
						"failing, restart Windows first. Its logs are the dd_*.log files in " + Path.GetTempPath() + ".";
			}
		}

		static string FindClang(Folders folders, string visualStudio, IReport report)
		{
			var candidates = new List<string> { Path.Combine(folders.LlvmDirectory, "bin"), Path.Combine(Util.ProgramFiles, @"LLVM\bin") };
			candidates.AddRange((System.Environment.GetEnvironmentVariable("PATH") ?? "").Split(';'));
			if (visualStudio != null)
			{
				candidates.Add(Path.Combine(visualStudio, @"VC\Tools\Llvm\x64\bin"));
				candidates.Add(Path.Combine(visualStudio, @"VC\Tools\Llvm\bin"));
			}
			foreach (string candidate in candidates.Select(c => c.Trim().Trim('"')).Where(c => c.Length > 0).Distinct(StringComparer.OrdinalIgnoreCase))
			{
				if (UsableClang(candidate, report))
					return candidate;
			}
			return null;
		}

		// clang.exe of at least MinimumClangMajor, with lld-link.exe next to it
		static bool UsableClang(string directory, IReport report)
		{
			string clang;
			try
			{
				clang = Path.Combine(directory, "clang.exe");
				if (!File.Exists(clang) || !File.Exists(Path.Combine(directory, "lld-link.exe")))
					return false;
			}
			catch (ArgumentException)
			{
				return false;
			}
			int major = ClangMajor(clang, report);
			if (major >= Pinned.MinimumClangMajor)
				return true;
			report.Log("Not using clang " + major + " in " + directory + ": version " + Pinned.MinimumClangMajor + " or later is needed.");
			return false;
		}

		static int ClangMajor(string clang, IReport report)
		{
			int major = 0;
			try
			{
				Util.Run(clang, "--version", null, null, report, delegate(string line)
				{
					Match version = Regex.Match(line, @"clang version (\d+)");
					if (version.Success && major == 0)
						major = int.Parse(version.Groups[1].Value);
				}, null);
			}
			catch (UserError)
			{
			}
			return major;
		}

		static bool HasVersion(string directory, string version)
		{
			string marker = Path.Combine(directory, ".launcher-version");
			return File.Exists(marker) && File.ReadAllText(marker).Trim() == version;
		}

		static void MarkVersion(string directory, string version)
		{
			File.WriteAllText(Path.Combine(directory, ".launcher-version"), version);
		}

		// clang and lld from LLVM's release, when none is installed (the
		// archive is large; only these two programs and clang's headers are
		// unpacked)
		static string InstallLlvm(Folders folders, IReport report)
		{
			string bin = Path.Combine(folders.LlvmDirectory, "bin");
			if (HasVersion(folders.LlvmDirectory, Pinned.LlvmVersion))
				return bin;
			string archive = Path.Combine(folders.Downloads, Pinned.LlvmFolder + ".tar.xz");
			Util.RequireSpace(folders.Root, 1400L << 20, "LLVM");
			report.Status("Downloading the clang compiler (about 820 MB)");
			Util.Download(Pinned.LlvmUrl, archive, report, Pinned.LlvmSha256);
			report.Status("Unpacking the clang compiler");
			string staging = folders.LlvmDirectory + ".new";
			Util.TryDeleteDirectory(staging);
			Tar.Extract(archive, staging, 1, new[]
			{
				Pinned.LlvmFolder + "/bin/clang.exe",
				Pinned.LlvmFolder + "/bin/lld-link.exe",
				Pinned.LlvmFolder + "/lib/clang",
			}, report);
			MarkVersion(staging, Pinned.LlvmVersion);
			Util.ReplaceDirectory(staging, folders.LlvmDirectory);
			File.Delete(archive);
			return bin;
		}

		// python.org's NuGet package is a complete Python that runs from a folder
		static void InstallPython(Folders folders, IReport report)
		{
			if (HasVersion(folders.PythonDirectory, Pinned.PythonVersion) && File.Exists(folders.PythonExe))
				return;
			string package = Path.Combine(folders.Downloads, "python." + Pinned.PythonVersion + ".nupkg");
			report.Status("Downloading Python");
			Util.Download(Pinned.PythonUrl, package, report, Pinned.PythonSha256);
			report.Status("Unpacking Python");
			string staging = folders.PythonDirectory + ".new";
			Util.TryDeleteDirectory(staging);
			using (ZipArchive zip = ZipFile.OpenRead(package))
			{
				foreach (ZipArchiveEntry entry in zip.Entries)
				{
					if (!entry.FullName.StartsWith("tools/") || entry.FullName.EndsWith("/"))
						continue;
					string target = Path.GetFullPath(Path.Combine(staging, Uri.UnescapeDataString(entry.FullName.Substring(6)).Replace('/', '\\')));
					if (!target.StartsWith(staging + "\\", StringComparison.OrdinalIgnoreCase))
						continue;
					Directory.CreateDirectory(Path.GetDirectoryName(target));
					entry.ExtractToFile(target, true);
				}
			}
			MarkVersion(staging, Pinned.PythonVersion);
			Util.ReplaceDirectory(staging, folders.PythonDirectory);
			File.Delete(package);
		}

		static void InstallNinja(Folders folders, IReport report)
		{
			if (HasVersion(folders.NinjaDirectory, Pinned.NinjaVersion) && File.Exists(folders.NinjaExe))
				return;
			string archive = Path.Combine(folders.Downloads, "ninja-win.zip");
			report.Status("Downloading ninja");
			Util.Download(Pinned.NinjaUrl, archive, report, Pinned.NinjaSha256);
			string staging = folders.NinjaDirectory + ".new";
			Util.TryDeleteDirectory(staging);
			Directory.CreateDirectory(staging);
			using (ZipArchive zip = ZipFile.OpenRead(archive))
				zip.GetEntry("ninja.exe").ExtractToFile(Path.Combine(staging, "ninja.exe"), true);
			MarkVersion(staging, Pinned.NinjaVersion);
			Util.ReplaceDirectory(staging, folders.NinjaDirectory);
			File.Delete(archive);
		}
	}

	/* ---------- the game's source */

	static class SourceTree
	{
		// The research notes are not needed to build, and their long paths
		// could pass Windows' 260 character limit.
		static bool Skipped(string relative)
		{
			return relative.StartsWith("research/", StringComparison.OrdinalIgnoreCase);
		}

		public static string LatestCommit()
		{
			string sha = Util.GetText(Pinned.CommitApiUrl, "application/vnd.github.sha").Trim();
			return Regex.IsMatch(sha, "^[0-9a-f]{40}$") ? sha : null;
		}

		// Downloads the branch; the commit is the zip's comment (GitHub writes
		// it there).
		public static string Download(Folders folders, IReport report, out string commit)
		{
			string zip = Path.Combine(folders.Downloads, "source.zip");
			Util.Download(Pinned.SourceZipUrl, zip, report, null);
			commit = ZipComment(zip).Trim();
			if (!Regex.IsMatch(commit, "^[0-9a-f]{40}$"))
				commit = "";
			return zip;
		}

		static string ZipComment(string path)
		{
			using (FileStream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
			{
				int tail = (int)Math.Min(stream.Length, 22 + 65535);
				byte[] buffer = new byte[tail];
				stream.Position = stream.Length - tail;
				Util.ReadUpTo(stream, buffer, tail);
				for (int i = tail - 22; i >= 0; i--)
				{
					if (buffer[i] == 0x50 && buffer[i + 1] == 0x4B && buffer[i + 2] == 5 && buffer[i + 3] == 6)
					{
						int length = BitConverter.ToUInt16(buffer, i + 20);
						if (i + 22 + length <= tail)
							return Encoding.ASCII.GetString(buffer, i + 22, length);
					}
				}
			}
			return "";
		}

		// Brings the game folder up to date: writes new and changed files
		// (unchanged ones keep their time, so ninja rebuilds only what
		// changed), and deletes files an earlier download installed that are
		// gone now. What the build and the launcher added (build\, xbox\,
		// build.ninja, ...) is in neither download and stays.
		public static void Sync(string zipPath, Folders folders, IReport report)
		{
			string game = Path.GetFullPath(folders.Game);
			Directory.CreateDirectory(game);
			var previous = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
			if (File.Exists(folders.SourceManifest))
				previous.UnionWith(File.ReadAllLines(folders.SourceManifest).Where(l => l.Length > 0));
			var current = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
			int written = 0, removed = 0;
			using (ZipArchive zip = ZipFile.OpenRead(zipPath))
			{
				long total = zip.Entries.Sum(e => e.Length), done = 0;
				foreach (ZipArchiveEntry entry in zip.Entries)
				{
					Util.ThrowIfCancelled(report);
					int slash = entry.FullName.IndexOf('/');
					string relative = slash < 0 ? "" : entry.FullName.Substring(slash + 1);
					if (relative.Length == 0 || Skipped(relative))
						continue;
					string target = Path.GetFullPath(Path.Combine(game, relative.Replace('/', '\\')));
					if (!target.StartsWith(game + "\\", StringComparison.OrdinalIgnoreCase))
						continue;
					if (entry.FullName.EndsWith("/"))
					{
						Directory.CreateDirectory(target);
						continue;
					}
					current.Add(relative);
					if (!SameContent(entry, target))
					{
						Directory.CreateDirectory(Path.GetDirectoryName(target));
						using (Stream input = entry.Open())
						using (FileStream output = new FileStream(target, FileMode.Create, FileAccess.Write))
							input.CopyTo(output);
						written++;
					}
					done += entry.Length;
					report.Progress(done, total);
				}
			}
			foreach (string old in previous.Where(p => !current.Contains(p)))
			{
				string file = Path.Combine(game, old.Replace('/', '\\'));
				if (File.Exists(file))
				{
					File.Delete(file);
					removed++;
				}
			}
			File.WriteAllLines(folders.SourceManifest, current.OrderBy(p => p, StringComparer.Ordinal));
			report.Log(string.Format("Source: {0} files, {1} new or changed, {2} removed.", current.Count, written, removed));
		}

		static bool SameContent(ZipArchiveEntry entry, string path)
		{
			FileInfo file = new FileInfo(path);
			if (!file.Exists || file.Length != entry.Length)
				return false;
			byte[] a = new byte[1 << 16], b = new byte[1 << 16];
			using (Stream zipped = entry.Open())
			using (FileStream existing = file.OpenRead())
			{
				while (true)
				{
					int read = Util.ReadUpTo(zipped, a, a.Length);
					if (Util.ReadUpTo(existing, b, read) != read)
						return false;
					for (int i = 0; i < read; i++)
					{
						if (a[i] != b[i])
							return false;
					}
					if (read < a.Length)
						return true;
				}
			}
		}
	}

	// The game loads maps of the PAL build only: source/cache/cache_files.c
	// and cache_files_windows.c check it. The NTSC build's maps load and play
	// the same, so for them the launcher adds that build to both checks in its
	// copy of the source, in the port's own HALO_LINUX code (the Xbox build is
	// unchanged). A source that has the NTSC build already is left as it is.
	static class SourcePatches
	{
		sealed class Change
		{
			public string File, Before, After;
		}

		static readonly Change[] Ntsc =
		{
			new Change
			{
				File = @"source\cache\cache_files.c",
				Before = "\tif (csstrcmp(header->build, \"" + Pinned.PalBuild + "\"))\n",
				After = "\tif (csstrcmp(header->build, \"" + Pinned.PalBuild + "\")\n" +
					"#ifdef HALO_LINUX\n" +
					"\t\t&& csstrcmp(header->build, \"" + Pinned.NtscBuild + "\")\n" +
					"#endif\n" +
					"\t\t)\n",
			},
			new Change
			{
				File = @"source\cache\cache_files_windows.c",
				Before = "\t\t\tif (strcmp(map_file->header.build, CACHE_FILE_BUILD_STRING) != 0)\n",
				After = "\t\t\tif (strcmp(map_file->header.build, CACHE_FILE_BUILD_STRING) != 0\n" +
					"#ifdef HALO_LINUX\n" +
					"\t\t\t\t&& strcmp(map_file->header.build, \"" + Pinned.NtscBuild + "\") != 0\n" +
					"#endif\n" +
					"\t\t\t\t)\n",
			},
		};

		// byte for byte: the files are changed only where they are replaced
		static readonly Encoding Bytes = Encoding.GetEncoding(28591);

		// Makes the game accept maps of the NTSC build. False, with nothing
		// changed, if the source no longer has the lines this changes.
		public static bool AcceptNtsc(Folders folders, IReport report)
		{
			// the file in game, and its new text
			var changed = new List<Tuple<string, string>>();
			foreach (Change change in Ntsc)
			{
				string path = Path.Combine(folders.Game, change.File);
				if (!File.Exists(path))
					return false;
				string text = Bytes.GetString(File.ReadAllBytes(path));
				if (text.Contains("\"" + Pinned.NtscBuild + "\""))
					continue;
				string newline = text.Contains("\r\n") ? "\r\n" : "\n";
				string before = change.Before.Replace("\n", newline);
				int at = text.IndexOf(before, StringComparison.Ordinal);
				if (at < 0 || text.IndexOf(before, at + 1, StringComparison.Ordinal) >= 0)
					return false;
				changed.Add(Tuple.Create(change.File,
					text.Substring(0, at) + change.After.Replace("\n", newline) + text.Substring(at + before.Length)));
			}
			foreach (var file in changed)
			{
				string path = Path.Combine(folders.Game, file.Item1);
				string temporary = path + ".launcher";
				File.WriteAllBytes(temporary, Bytes.GetBytes(file.Item2));
				File.Replace(temporary, path, null);
				report.Log("Added the American version's build " + Pinned.NtscBuild + " to the map check in game\\" + file.Item1 + ".");
			}
			if (changed.Count == 0)
				report.Log("The source accepts the American version's maps already.");
			return true;
		}
	}

	/* ---------- building and starting the game */

	static class GameBuild
	{
		const string TryAgain = "Try again; if it keeps failing, copy the details and report them at " + Pinned.RepositoryUrl + "/issues.";

		// a running halo.exe cannot be replaced: the link would fail
		public static void RequireNotRunning(Folders folders)
		{
			foreach (Process process in Process.GetProcessesByName("halo"))
			{
				bool ours = false;
				try
				{
					ours = string.Equals(Path.GetFullPath(process.MainModule.FileName), Path.GetFullPath(folders.HaloExe),
						StringComparison.OrdinalIgnoreCase);
				}
				catch (Exception)
				{
					// another user's or an elevated process
				}
				process.Dispose();
				if (ours)
					throw new UserError("Halo is running from " + folders.Root + ". Close it, then try again.");
			}
		}

		// whether this version of configure.py has the option
		static bool ConfigureHas(Folders folders, string option)
		{
			try
			{
				return File.ReadAllText(Path.Combine(folders.Game, "configure.py")).Contains("\"" + option + "\"");
			}
			catch (IOException)
			{
				return false;
			}
		}

		public static void Run(Folders folders, Toolchain tools, bool developer, IReport report)
		{
			Dictionary<string, string> environment = tools.BuildEnvironment(folders);
			report.Status("Preparing the build");
			string arguments = "configure.py" + (developer ? "" : " --release");
			// Full link-time optimisation, configure.py's default, lets the
			// compiler act on undefined behaviour in the decompiled code
			// across files: object_reconnect_to_map (source/objects/objects.c)
			// reads a local through a pointer after the local's block has
			// ended, the stores to it are dropped, objects get a garbage
			// cluster, and the first campaign level crashes. Compiled one file
			// at a time, as before, the game runs.
			if (ConfigureHas(folders, "--lto"))
				arguments += " --lto=off";
			int code = Util.Run(folders.PythonExe, arguments, folders.Game, environment, report);
			if (code != 0)
				throw new UserError("Preparing the build failed (code " + code + "). " + TryAgain);

			report.Status("Building Halo (this takes a minute or two)");
			Regex step = new Regex(@"^\[(\d+)/(\d+)\]");
			bool noWindowsTarget = false;
			code = Util.Run(folders.NinjaExe, "windows", folders.Game, environment, report, delegate(string line)
			{
				Match match = step.Match(line);
				if (match.Success)
					report.Progress(long.Parse(match.Groups[1].Value), long.Parse(match.Groups[2].Value));
				if (line.Contains("unknown target 'windows'"))
					noWindowsTarget = true;
				report.Log(line);
			}, null);
			if (noWindowsTarget)
			{
				throw new UserError("Preparing the build couldn't download SDL3 from GitHub. Check your internet connection, then try again.");
			}
			if (code != 0)
				throw new UserError("Building Halo failed (code " + code + "). " + TryAgain);
			if (!folders.GameBuilt)
				throw new UserError("The build finished, but " + folders.HaloExe + " is missing.");
		}
	}

	static class GameLauncher
	{
		// halo.exe is a console program: it runs with its console hidden and
		// its output in logs\game.log (cmd.exe holds the file, so the game
		// keeps running if the launcher closes).
		public static Process Start(Folders folders, Settings settings)
		{
			Directory.CreateDirectory(folders.Logs);
			ProcessStartInfo info = new ProcessStartInfo(Path.Combine(Util.System32, "cmd.exe"),
				"/d /s /c \"\"" + folders.HaloExe + "\" > \"" + folders.GameLog + "\" 2>&1\"");
			info.UseShellExecute = false;
			info.CreateNoWindow = true;
			info.WorkingDirectory = folders.Game;
			settings.ApplyTo(info.EnvironmentVariables, folders);
			return Process.Start(info);
		}
	}

	/* ---------- the launcher's own updates */

	// Each release of the launcher (Pinned.LauncherReleases) carries
	// HaloLauncher.exe and launcher.txt, which names the newest version:
	//
	//   version=1.2.0.0
	//   sha256=<SHA-256 of HaloLauncher.exe>
	//   url=https://github.com/.../releases/download/<tag>/HaloLauncher.exe
	//
	// When the window opens, a newer version is offered; taken, it is
	// downloaded and checked, the running exe is renamed to .old (Windows
	// allows renaming a running program; the next start deletes it), the new
	// one takes its name and starts. HALO_LAUNCHER_UPDATES points the check at
	// another launcher.txt, to try a release before publishing it.
	static class SelfUpdate
	{
		public sealed class Release
		{
			public Version Version;
			public string Url;
			public string Sha256;
		}

		public static Version Current
		{
			get { return Assembly.GetExecutingAssembly().GetName().Version; }
		}

		static string Override
		{
			get { return Environment.GetEnvironmentVariable("HALO_LAUNCHER_UPDATES") ?? ""; }
		}

		// the newest release when it is newer than this launcher, else null
		// (also without a connection)
		public static Release Check()
		{
			string text;
			try
			{
				text = Util.GetText(Override.Length > 0 ? Override : Pinned.LauncherReleases + "/latest/download/launcher.txt", null);
			}
			catch (Exception)
			{
				return null;
			}
			var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
			foreach (string line in text.Split('\n'))
			{
				string trimmed = line.Trim();
				int equals = trimmed.IndexOf('=');
				if (!trimmed.StartsWith("#") && equals > 0)
					values[trimmed.Substring(0, equals).Trim()] = trimmed.Substring(equals + 1).Trim();
			}
			string version, url, sha256;
			Version parsed;
			Uri uri;
			if (!values.TryGetValue("version", out version) || !Version.TryParse(version, out parsed) || parsed <= Current)
				return null;
			if (!values.TryGetValue("sha256", out sha256) || !Regex.IsMatch(sha256, "^[0-9a-fA-F]{64}$"))
				return null;
			// https, or a local test server with HALO_LAUNCHER_UPDATES
			if (!values.TryGetValue("url", out url) || !Uri.TryCreate(url, UriKind.Absolute, out uri) ||
				!(uri.Scheme == Uri.UriSchemeHttps || (Override.Length > 0 && uri.IsLoopback)))
				return null;
			return new Release { Version = parsed, Url = url, Sha256 = sha256.ToLowerInvariant() };
		}

		// Downloads the release, checks it, and puts it in place of exe.
		public static void Apply(Release release, string exe, IReport report)
		{
			string fresh = exe + ".new";
			string old = exe + ".old";
			Util.Download(release.Url, fresh, report, release.Sha256);
			if (File.Exists(old))
				File.Delete(old);
			File.Move(exe, old);
			try
			{
				File.Move(fresh, exe);
			}
			catch (Exception)
			{
				File.Move(old, exe);
				throw;
			}
		}

		// the exe an update replaced, deleted once it has stopped running
		// (right after an update, it may still be closing)
		public static void Tidy(string exe)
		{
			string old = exe + ".old";
			for (int attempt = 0; attempt < 20 && File.Exists(old); attempt++)
			{
				try
				{
					File.Delete(old);
				}
				catch (Exception)
				{
					Thread.Sleep(500);
				}
			}
		}

		// Checks in the background; when there is a newer version and the
		// window isn't busy (installing, updating, playing), asks, and on yes
		// updates and starts the new launcher in place of this one.
		public static void Offer(Form window, Func<bool> busy)
		{
			string exe = Application.ExecutablePath;
			var thread = new Thread(delegate()
			{
				Tidy(exe);
				Release release = Check();
				if (release == null)
					return;
				try
				{
					window.BeginInvoke(new Action(delegate
					{
						if (window.IsDisposed || busy())
							return;
						if (MessageBox.Show(window, "A new version of the Halo CE Universal launcher is available: " + release.Version +
							" (this is " + Current + ").\r\n\r\nUpdate it now? It takes a few seconds, then the launcher starts again.",
							"Halo CE Universal", MessageBoxButtons.YesNo, MessageBoxIcon.Information) != DialogResult.Yes)
							return;
						Cursor cursor = window.Cursor;
						window.Cursor = Cursors.WaitCursor;
						try
						{
							Apply(release, exe, new LogOnlyReport(null));
						}
						catch (Exception error)
						{
							window.Cursor = cursor;
							MessageBox.Show(window, "Couldn't update the launcher: " + error.Message + "\r\n\r\nDownload the new version from " +
								Pinned.LauncherReleases + " instead.", "Halo CE Universal", MessageBoxButtons.OK, MessageBoxIcon.Warning);
							return;
						}
						Process.Start(exe);
						// nothing is under way (busy() said so): no questions on the way out
						Environment.Exit(0);
					}));
				}
				catch (InvalidOperationException)
				{
					// the window closed meanwhile
				}
			});
			thread.IsBackground = true;
			thread.Start();
		}
	}

	/* ---------- the whole installation */

	static class Installer
	{
		// Installs or repairs; with updateSource, also brings the source up to
		// date first.
		public static void Run(Settings settings, IReport report, bool updateSource, bool shortcuts)
		{
			string root;
			string unusable = Folders.Problem(settings.Root, out root);
			if (unusable != null)
				throw new UserError(unusable);
			settings.Root = root;
			Folders folders = new Folders(root);
			report.Status("Getting ready");
			report.Log("Install folder: " + folders.Root);
			Directory.CreateDirectory(folders.Root);
			settings.Save();
			// what this run unpacks goes here, and only this folder is deleted
			// at the end (the downloads next to it are kept if a step fails,
			// and the pinned ones are reused); earlier runs that were stopped
			// before they could clean up left theirs
			if (Directory.Exists(folders.Downloads))
			{
				foreach (string old in Directory.GetDirectories(folders.Downloads, "run-*"))
					Util.TryDeleteDirectory(old);
			}
			string scratch = Util.NewDirectory(folders.Downloads, "run");

			GameDataSource data = null;
			try
			{
				// the player's files are checked before anything is downloaded
				report.Step(InstallStep.CheckFiles);

				// other game data than the installed data replaces it entirely
				bool replaceData = settings.DataInput.Length > 0 &&
					!string.Equals(settings.DataInput, settings.InstalledData, StringComparison.OrdinalIgnoreCase);
				if (GameData.IsInstalled(folders.Data) && !replaceData)
				{
					report.Log("Game data: installed.");
				}
				else
				{
					if (settings.DataInput.Length == 0)
						throw new UserError("Choose your Halo game files first.");
					report.Status("Checking your Halo game files");
					data = GameData.Open(settings.DataInput, scratch, report, true);
					if (data == null)
						throw new UserError("No Halo game files were found in " + settings.DataInput + ".");
					string problem = GameData.Problem(data.Maps);
					if (problem != null)
						throw new UserError(problem);
					report.Log(data.Summary());
				}

				GameBuild.RequireNotRunning(folders);
				long gameBytes = (600L << 20) + (data == null ? 0 : data.BytesToCopy(folders.Data, replaceData));
				Util.RequireSpace(folders.Root, gameBytes, "the installation");
				// Microsoft's build tools go on the Windows drive, and running out
				// of space there fails their installer half-way through
				if (!Toolchain.HasBuildTools())
				{
					bool sameDrive = Util.SameDrive(folders.Root, Util.ProgramFilesX86);
					Util.RequireSpace(Util.ProgramFilesX86, Pinned.BuildToolsSpace + (sameDrive ? gameBytes : 0),
						sameDrive ? "Microsoft's C++ build tools and Halo" : "Microsoft's C++ build tools");
				}
				report.Step(InstallStep.GetTools);
				Toolchain tools = Toolchain.Prepare(folders, settings.Clang, report);
				if (tools.InstalledBuildTools)
				{
					settings.InstalledBuildTools = true;
					settings.Save();
				}

				report.Step(InstallStep.GetSource);
				string commit = settings.Commit;
				if (updateSource || !File.Exists(Path.Combine(folders.Game, "configure.py")))
				{
					report.Status("Downloading Halo's source code");
					string zip = SourceTree.Download(folders, report, out commit);
					report.Status("Unpacking Halo's source code");
					report.Log("Commit " + commit);
					SourceTree.Sync(zip, folders, report);
					File.Delete(zip);
				}
				// the native builds compile against port/include/xdk, not the Xbox
				// SDK (whose headers earlier launchers copied to xbox\include,
				// which the build now leaves out)
				if (!File.Exists(Path.Combine(folders.Game, Pinned.CleanSdkHeader)))
				{
					throw new UserError("This version of Halo's source code has no " + Pinned.CleanSdkHeader + ", so it would need the Xbox " +
						"development kit, which this launcher doesn't handle any more. Try again later, or look for news at " + Pinned.RepositoryUrl + ".");
				}
				// on every run: an update brings back the unchanged files
				string build = data != null ? data.Build : GameData.InstalledBuild(folders.Data);
				if (build == Pinned.NtscBuild)
				{
					report.Status("Making Halo accept the American version");
					if (!SourcePatches.AcceptNtsc(folders, report))
					{
						throw new UserError("Halo's source code has changed where it checks the version of the game files, so this launcher " +
							"can't make it accept the American (NTSC) version any more. Look for a newer launcher at " + Pinned.RepositoryUrl +
							", or use the European (PAL) version of the game.");
					}
				}

				report.Step(InstallStep.CopyFiles);
				if (data != null)
				{
					report.Status("Copying your game files (" + Util.Size(data.BytesToCopy(folders.Data, replaceData)) + ")");
					data.CopyTo(folders.Data, report, replaceData);
					if (!GameData.IsInstalled(folders.Data))
						throw new UserError("Copying your game files didn't work: the copy in " + folders.Data + " is incomplete. Try again.");
					settings.InstalledData = settings.DataInput;
					settings.Save();
				}

				report.Step(InstallStep.Build);
				GameBuild.Run(folders, tools, settings.DeveloperBuild, report);
				// the version is recorded only once it is built, so that a
				// failed update is tried again
				settings.Commit = commit;
				settings.Save();
			}
			finally
			{
				if (data != null)
					data.Dispose();
				Util.TryDeleteDirectory(scratch);
			}

			report.Status("Finishing up");
			CopyLauncher(folders, report);
			try
			{
				AppIcon.Save(folders.IconFile);
			}
			catch (Exception error)
			{
				report.Log("Could not write the icon: " + error.Message);
			}
			if (shortcuts)
			{
				try
				{
					Shortcuts.Create(folders);
					report.Log("Added Halo CE Universal to the Start menu and the desktop.");
				}
				catch (Exception error)
				{
					report.Log("Could not add shortcuts: " + error.Message);
				}
				// and to Windows' list of installed apps, which can uninstall it
				try
				{
					Uninstaller.Register(folders, settings);
				}
				catch (Exception error)
				{
					report.Log("Could not add Halo CE Universal to Windows' installed apps: " + error.Message);
				}
			}
			settings.Save();
			try
			{
				Directory.Delete(folders.Downloads); // only when empty
			}
			catch (IOException)
			{
			}
			catch (UnauthorizedAccessException)
			{
			}
			report.Status("Halo is installed and ready to play.");
		}

		[DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
		static extern bool DeleteFile(string path);

		// the installed copy of the launcher, which the shortcuts start
		static void CopyLauncher(Folders folders, IReport report)
		{
			string self = Path.GetFullPath(Application.ExecutablePath);
			if (string.Equals(self, Path.GetFullPath(folders.LauncherExe), StringComparison.OrdinalIgnoreCase))
				return;
			try
			{
				File.Copy(self, folders.LauncherExe, true);
				// A downloaded launcher carries Windows' "came from the internet"
				// mark, and a copy keeps it: Windows would warn at every start
				// from the shortcuts. The player ran it already.
				DeleteFile(folders.LauncherExe + ":Zone.Identifier");
			}
			catch (Exception error)
			{
				report.Log("Could not copy the launcher to " + folders.LauncherExe + ": " + error.Message);
			}
		}
	}

	static class Shortcuts
	{
		const string Name = "Halo CE Universal.lnk";

		static IEnumerable<string> Places()
		{
			yield return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs), Name);
			yield return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), Name);
		}

		public static void Create(Folders folders)
		{
			string target = File.Exists(folders.LauncherExe) ? folders.LauncherExe : Application.ExecutablePath;
			foreach (string place in Places())
			{
				Use(place, delegate(object link, Type type)
				{
					type.InvokeMember("TargetPath", BindingFlags.SetProperty, null, link, new object[] { target });
					type.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, link, new object[] { folders.Root });
					type.InvokeMember("Description", BindingFlags.SetProperty, null, link, new object[] { "Play Halo: Combat Evolved (halo-ce-universal)" });
					if (File.Exists(folders.IconFile))
						type.InvokeMember("IconLocation", BindingFlags.SetProperty, null, link, new object[] { folders.IconFile + ",0" });
					type.InvokeMember("Save", BindingFlags.InvokeMethod, null, link, null);
				});
			}
		}

		// removes the shortcuts that start this install's launcher
		public static void Remove(Folders folders)
		{
			foreach (string place in Places().Where(File.Exists))
			{
				string target = null;
				Use(place, delegate(object link, Type type)
				{
					target = type.InvokeMember("TargetPath", BindingFlags.GetProperty, null, link, null) as string;
				});
				if (string.Equals(target, folders.LauncherExe, StringComparison.OrdinalIgnoreCase))
					File.Delete(place);
			}
		}

		static void Use(string path, Action<object, Type> use)
		{
			Type shellType = Type.GetTypeFromProgID("WScript.Shell");
			object shell = Activator.CreateInstance(shellType);
			try
			{
				object link = shellType.InvokeMember("CreateShortcut", BindingFlags.InvokeMethod, null, shell, new object[] { path });
				try
				{
					use(link, link.GetType());
				}
				finally
				{
					Marshal.FinalReleaseComObject(link);
				}
			}
			finally
			{
				Marshal.FinalReleaseComObject(shell);
			}
		}
	}

	/* ---------- Windows' list of installed apps, and uninstalling */

	static class Uninstaller
	{
		const string AppsKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall\HaloCEUniversal";
		const string LauncherKey = @"Software\HaloCEUniversal";

		public static void Register(Folders folders, Settings settings)
		{
			long size = 0;
			foreach (string folder in new[] { folders.Game, folders.Data, folders.Tools })
			{
				try
				{
					size += new DirectoryInfo(folder).EnumerateFiles("*", SearchOption.AllDirectories).Sum(f => f.Length);
				}
				catch (Exception)
				{
				}
			}
			using (RegistryKey key = Registry.CurrentUser.CreateSubKey(AppsKey))
			{
				key.SetValue("DisplayName", "Halo CE Universal");
				key.SetValue("DisplayIcon", File.Exists(folders.IconFile) ? folders.IconFile : folders.LauncherExe);
				key.SetValue("DisplayVersion", settings.Commit.Length >= 7 ? settings.Commit.Substring(0, 7) : "");
				key.SetValue("Publisher", "halo-ce-universal (github.com/" + Pinned.Repository + ")");
				key.SetValue("URLInfoAbout", Pinned.RepositoryUrl);
				key.SetValue("InstallLocation", folders.Root);
				key.SetValue("UninstallString", "\"" + folders.LauncherExe + "\" --uninstall");
				key.SetValue("EstimatedSize", (int)Math.Min(int.MaxValue, size / 1024), RegistryValueKind.DWord);
				key.SetValue("NoModify", 1, RegistryValueKind.DWord);
				key.SetValue("NoRepair", 1, RegistryValueKind.DWord);
			}
		}

		// Asks, then removes the installation: the game, its tools, the copy of
		// the game data, the shortcuts and the registry entries. Saved games
		// stay. Returns true if it was removed (the caller then exits, and the
		// launcher's own file goes a moment later).
		public static bool Run(IWin32Window owner, Settings settings)
		{
			const string title = "Uninstall Halo CE Universal";
			Folders folders = settings.Folders();
			if (!settings.Exists || !Folders.IsOwn(folders.Root))
			{
				MessageBox.Show(owner, "Halo CE Universal isn't installed in " + folders.Root + ".", title, MessageBoxButtons.OK, MessageBoxIcon.Information);
				return false;
			}
			string tools = settings.InstalledBuildTools
				? "\r\n\r\nMicrosoft's C++ build tools, which were installed for Halo, stay: remove \"Visual Studio Build Tools 2022\" in Windows' Settings > Apps if nothing else needs them."
				: "";
			if (MessageBox.Show(owner, "Remove Halo CE Universal from this PC?\r\n\r\nThis deletes the game, its tools and the copy of your game files in " +
				folders.Root + ". Your saved games stay (in %APPDATA%\\halo)." + tools, title, MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
				return false;
			try
			{
				GameBuild.RequireNotRunning(folders);
				RequireNoOtherLauncher(folders);
			}
			catch (UserError error)
			{
				MessageBox.Show(owner, error.Message, title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
				return false;
			}
			if (!Remove(folders))
			{
				MessageBox.Show(owner, "Some files in " + folders.Root + " couldn't be removed, maybe because a program still " +
					"uses them. Restart Windows, then uninstall again.", title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
				return false;
			}
			MessageBox.Show(owner, "Halo CE Universal was removed.", title, MessageBoxButtons.OK, MessageBoxIcon.Information);
			RemoveLauncherLater(folders);
			return true;
		}

		// another launcher window of this install may be building
		static void RequireNoOtherLauncher(Folders folders)
		{
			int self = Process.GetCurrentProcess().Id;
			foreach (Process process in Process.GetProcessesByName("HaloLauncher"))
			{
				bool other = false;
				try
				{
					other = process.Id != self && string.Equals(Path.GetFullPath(process.MainModule.FileName),
						Path.GetFullPath(folders.LauncherExe), StringComparison.OrdinalIgnoreCase);
				}
				catch (Exception)
				{
				}
				process.Dispose();
				if (other)
					throw new UserError("Halo CE Universal is open in another window. Close it, then uninstall again.");
			}
		}

		// Everything but the launcher's own file, which may be running. Only
		// when the folders are gone are the shortcuts and the registry entries
		// removed too, so that a failed uninstall can be done again.
		public static bool Remove(Folders folders)
		{
			string[] owned = { folders.Game, folders.Data, folders.Tools, folders.Downloads, folders.Logs };
			foreach (string folder in owned)
				Util.TryDeleteDirectory(folder);
			if (owned.Any(Directory.Exists))
				return false;
			foreach (string file in new[] { folders.SettingsFile, folders.IconFile })
			{
				try
				{
					File.Delete(file);
				}
				catch (Exception)
				{
				}
			}
			try
			{
				Shortcuts.Remove(folders);
			}
			catch (Exception)
			{
			}
			// the registry entries, if they are about this install
			try
			{
				if (Points(AppsKey, "InstallLocation", folders.Root))
					Registry.CurrentUser.DeleteSubKeyTree(AppsKey, false);
				if (Points(LauncherKey, "InstallRoot", folders.Root))
					Registry.CurrentUser.DeleteSubKeyTree(LauncherKey, false);
			}
			catch (Exception)
			{
			}
			return true;
		}

		static bool Points(string keyName, string valueName, string root)
		{
			using (RegistryKey key = Registry.CurrentUser.OpenSubKey(keyName))
			{
				string value = key == null ? null : key.GetValue(valueName) as string;
				return value != null && string.Equals(value.TrimEnd('\\'), root.TrimEnd('\\'), StringComparison.OrdinalIgnoreCase);
			}
		}

		// the launcher's own file (in use now) and the then empty folder go
		// once it has exited
		public static void RemoveLauncherLater(Folders folders)
		{
			var info = new ProcessStartInfo(Path.Combine(Util.System32, "cmd.exe"),
				"/d /c ping -n 4 127.0.0.1 >nul & del /f /q \"" + folders.LauncherExe + "\" & rmdir \"" + folders.Root + "\"");
			info.UseShellExecute = false;
			info.CreateNoWindow = true;
			info.WorkingDirectory = Path.GetTempPath();
			Process.Start(info).Dispose();
		}
	}

	/* ---------- the look of the windows */

	static class Look
	{
		public static readonly Color Dark = Color.FromArgb(0x16, 0x20, 0x2A);
		public static readonly Color OnDark = Color.FromArgb(0xF2, 0xF5, 0xF7);
		public static readonly Color OnDarkMuted = Color.FromArgb(0x8F, 0xA1, 0xAF);
		public static readonly Color Paper = Color.White;
		public static readonly Color Bar = Color.FromArgb(0xF3, 0xF4, 0xF6);
		public static readonly Color Ink = Color.FromArgb(0x1F, 0x23, 0x28);
		public static readonly Color Muted = Color.FromArgb(0x5E, 0x69, 0x73);
		public static readonly Color Good = Color.FromArgb(0x1B, 0x7F, 0x3B);
		public static readonly Color Bad = Color.FromArgb(0xB4, 0x2B, 0x1F);
		public static readonly Color Busy = Color.FromArgb(0x1F, 0x6F, 0xC0);
		public static readonly Color Accent = Color.FromArgb(0x10, 0x7C, 0x10);
		public static readonly Color Line = Color.FromArgb(0xB8, 0xC0, 0xC8);
		public static readonly Color Note = Color.FromArgb(0xFF, 0xF4, 0xD6);
		public static readonly Color Hover = Color.FromArgb(0xE8, 0xF2, 0xFC);

		// state marks, drawn in Segoe UI Symbol
		public const string Tick = "\u2714";
		public const string Cross = "\u2716";
		public const string Dot = "\u25CF";
		public const string Ring = "\u25CB";

		public static Font Regular(float size) { return new Font("Segoe UI", size); }
		public static Font Bold(float size) { return new Font("Segoe UI Semibold", size); }
		public static Font Symbols(float size) { return new Font("Segoe UI Symbol", size); }

		// text that wraps at width
		public static Label Text(string text, float size, Color color, int width)
		{
			return new Label
			{
				Text = text,
				Font = Regular(size),
				ForeColor = color,
				AutoSize = true,
				MaximumSize = new Size(width, 0),
				Margin = new Padding(0, 0, 0, 10),
			};
		}

		// the one button that matters on a page
		public static Button Primary(string text, float size)
		{
			var button = new Button { Text = text, Font = Bold(size), ForeColor = Color.White, BackColor = Accent, FlatStyle = FlatStyle.Flat, Cursor = Cursors.Hand };
			button.FlatAppearance.BorderSize = 0;
			button.FlatAppearance.MouseOverBackColor = Color.FromArgb(0x17, 0x93, 0x17);
			button.FlatAppearance.MouseDownBackColor = Color.FromArgb(0x0B, 0x5E, 0x0B);
			button.EnabledChanged += delegate { button.BackColor = button.Enabled ? Accent : Color.FromArgb(0xC9, 0xCF, 0xD5); };
			return button;
		}

		public static Button Plain(string text)
		{
			return new Button { Text = text, AutoSize = true, MinimumSize = new Size(110, 34), Font = Regular(9.5F), Margin = new Padding(4, 0, 4, 0) };
		}
	}

	// The launcher's icon, a ring seen at an angle on a dark tile, drawn here
	// so that there is no image file to keep with the source.
	static class AppIcon
	{
		static Icon window;

		public static Bitmap Draw(int size)
		{
			var bitmap = new Bitmap(size, size, PixelFormat.Format32bppArgb);
			using (Graphics g = Graphics.FromImage(bitmap))
			{
				g.SmoothingMode = SmoothingMode.AntiAlias;
				g.PixelOffsetMode = PixelOffsetMode.HighQuality;
				using (GraphicsPath tile = Rounded(new RectangleF(0, 0, size, size), size * 0.2f))
				using (var fill = new LinearGradientBrush(new PointF(0, 0), new PointF(0, size), Color.FromArgb(0x27, 0x41, 0x55), Color.FromArgb(0x0B, 0x14, 0x1B)))
					g.FillPath(fill, tile);
				float sun = size * 0.17f;
				using (var light = new SolidBrush(Color.FromArgb(0xFF, 0xD9, 0x8C)))
					g.FillEllipse(light, size * 0.64f - sun / 2, size * 0.29f - sun / 2, sun, sun);
				GraphicsState state = g.Save();
				g.TranslateTransform(size / 2f, size * 0.54f);
				g.RotateTransform(-22);
				float width = size * 0.84f, height = size * 0.33f;
				using (var ring = new Pen(Color.FromArgb(0xB8, 0xEC, 0xFF), Math.Max(1.5f, size * 0.085f)))
					g.DrawEllipse(ring, -width / 2, -height / 2, width, height);
				g.Restore(state);
			}
			return bitmap;
		}

		static GraphicsPath Rounded(RectangleF area, float radius)
		{
			var path = new GraphicsPath();
			float d = radius * 2;
			path.AddArc(area.X, area.Y, d, d, 180, 90);
			path.AddArc(area.Right - d, area.Y, d, d, 270, 90);
			path.AddArc(area.Right - d, area.Bottom - d, d, d, 0, 90);
			path.AddArc(area.X, area.Bottom - d, d, d, 90, 90);
			path.CloseFigure();
			return path;
		}

		public static Icon Window
		{
			get
			{
				if (window == null)
				{
					using (Bitmap bitmap = Draw(64))
						window = Icon.FromHandle(bitmap.GetHicon());
				}
				return window;
			}
		}

		// an .ico of PNG images, for the shortcuts and Windows' list of apps
		public static void Save(string path)
		{
			int[] sizes = { 256, 64, 48, 32, 24, 16 };
			var images = new List<byte[]>();
			foreach (int size in sizes)
			{
				using (Bitmap bitmap = Draw(size))
				using (var png = new MemoryStream())
				{
					bitmap.Save(png, ImageFormat.Png);
					images.Add(png.ToArray());
				}
			}
			using (var writer = new BinaryWriter(File.Create(path)))
			{
				writer.Write((ushort)0);
				writer.Write((ushort)1);
				writer.Write((ushort)sizes.Length);
				int offset = 6 + 16 * sizes.Length;
				for (int i = 0; i < sizes.Length; i++)
				{
					writer.Write((byte)(sizes[i] % 256)); // 0 means 256
					writer.Write((byte)(sizes[i] % 256));
					writer.Write((byte)0);
					writer.Write((byte)0);
					writer.Write((ushort)1);
					writer.Write((ushort)32);
					writer.Write(images[i].Length);
					writer.Write(offset);
					offset += images[i].Length;
				}
				foreach (byte[] image in images)
					writer.Write(image);
			}
		}
	}

	static class KnownFolders
	{
		[DllImport("shell32.dll", CharSet = CharSet.Unicode)]
		static extern int SHGetKnownFolderPath([MarshalAs(UnmanagedType.LPStruct)] Guid id, uint flags, IntPtr token, out string path);

		static string Downloads()
		{
			try
			{
				string path;
				if (SHGetKnownFolderPath(new Guid("374DE290-123F-4565-9164-39C4925E467B"), 0, IntPtr.Zero, out path) == 0)
					return path;
			}
			catch (Exception)
			{
			}
			return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads");
		}

		// where downloaded files usually are
		public static List<string> Usual()
		{
			return new[]
			{
				Downloads(),
				Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),
				Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),
			}.Where(p => !string.IsNullOrEmpty(p) && Directory.Exists(p)).Distinct(StringComparer.OrdinalIgnoreCase).ToList();
		}
	}

	static class Help
	{
		const string Title = "Halo CE Universal";

		public static void ShowControls(IWin32Window owner)
		{
			MessageBox.Show(owner,
				"Keyboard and mouse play as controller 1; a gamepad works too.\r\n\r\n" +
				"W A S D\tmove\r\n" +
				"Mouse\taim\r\n" +
				"Left button\tfire\r\n" +
				"Right button, G\tgrenade\r\n" +
				"Space, Enter\tjump, accept\r\n" +
				"F, Backspace, mouse 4\tmelee, back\r\n" +
				"E, R\taction, reload\r\n" +
				"Tab, wheel\tswitch weapon\r\n" +
				"Q\tflashlight\r\n" +
				"X\tblack button\r\n" +
				"Left Ctrl, C\tcrouch\r\n" +
				"Z, middle button\tzoom\r\n" +
				"Arrows\tD-pad\r\n" +
				"Esc\tpause menu (Start)\r\n" +
				"F1\tBack\r\n" +
				"`\tdeveloper console\r\n" +
				"F12\tfree or capture the mouse\r\n" +
				"F11\tfullscreen or window",
				Title + ": controls", MessageBoxButtons.OK, MessageBoxIcon.Information);
		}

		public static void Show(IWin32Window owner, Folders folders)
		{
			MessageBox.Show(owner,
				"Play starts Halo. You can also start it with Halo CE Universal on your desktop or in the Start menu.\r\n\r\n" +
				"Check for updates gets the newest version of the game's code and rebuilds Halo (a minute or two).\r\n\r\n" +
				"Settings changes the window size, the mouse, the sound and the language.\r\n\r\n" +
				"If something stops working, More > Repair Halo checks everything and builds it again. More > Open the log " +
				"shows what the launcher did, and Copy details (after a problem) copies it for asking for help at " +
				Pinned.RepositoryUrl + "/issues.\r\n\r\n" +
				"Halo is installed in " + folders.Root + ". Saved games are in %APPDATA%\\halo.",
				Title + ": help", MessageBoxButtons.OK, MessageBoxIcon.Information);
		}
	}

	/* ---------- showing a long job */

	// Shows a long job: its steps (in setup), what it is doing, a progress
	// bar, and the whole log behind "Show details". It is the job's IReport.
	sealed class ProgressPanel : UserControl, IReport
	{
		static readonly string[] StepNames =
		{
			"Check your files",
			"Get the build tools",
			"Download Halo's source code",
			"Copy your files",
			"Build Halo",
		};

		readonly bool withSteps;
		readonly Label[] marks = new Label[StepNames.Length];
		readonly Label[] names = new Label[StepNames.Length];
		readonly Font nameFont = Look.Regular(10.5F);
		readonly Font currentFont = Look.Bold(10.5F);
		readonly Label statusLabel = new Label();
		readonly Label amountLabel = new Label();
		readonly ProgressBar bar = new ProgressBar();
		readonly LinkLabel detailsLink = new LinkLabel();
		readonly LinkLabel copyLink = new LinkLabel();
		readonly TextBox logBox = new TextBox();
		readonly System.Windows.Forms.Timer timer = new System.Windows.Forms.Timer();
		readonly ConcurrentQueue<string> pending = new ConcurrentQueue<string>();
		readonly LinkedList<string> recent = new LinkedList<string>();
		readonly object fileLock = new object();
		StreamWriter file;
		Thread worker;
		volatile bool cancelRequested;
		volatile string status = "";
		long done, total;
		volatile int step = -1;
		int failedStep = -1;

		// after the job: null if it worked, else what went wrong
		public event Action<string> Finished;

		public ProgressPanel(bool withSteps)
		{
			this.withSteps = withSteps;
			BackColor = Look.Paper;
			var grid = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1 };
			grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
			if (withSteps)
			{
				var list = new TableLayoutPanel { AutoSize = true, ColumnCount = 2, Margin = new Padding(0, 0, 0, 12) };
				list.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
				list.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
				for (int i = 0; i < StepNames.Length; i++)
				{
					marks[i] = new Label { AutoSize = true, Font = Look.Symbols(11F), Margin = new Padding(0, 3, 8, 3) };
					names[i] = new Label { AutoSize = true, Font = nameFont, Text = StepNames[i], Margin = new Padding(0, 3, 0, 3) };
					list.RowCount++;
					list.RowStyles.Add(new RowStyle(SizeType.AutoSize));
					list.Controls.Add(marks[i], 0, i);
					list.Controls.Add(names[i], 1, i);
				}
				AddRow(grid, list, SizeType.AutoSize);
				PaintSteps();
			}
			statusLabel.AutoSize = true;
			statusLabel.Font = Look.Regular(10F);
			statusLabel.ForeColor = Look.Ink;
			statusLabel.Margin = new Padding(0, 0, 0, 6);
			AddRow(grid, statusLabel, SizeType.AutoSize);
			var barRow = new TableLayoutPanel { AutoSize = true, Dock = DockStyle.Fill, ColumnCount = 2, Margin = new Padding(0) };
			barRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
			barRow.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			bar.Dock = DockStyle.Fill;
			bar.Height = 16;
			bar.Maximum = 1000;
			amountLabel.AutoSize = true;
			amountLabel.MinimumSize = new Size(56, 0);
			amountLabel.TextAlign = ContentAlignment.MiddleRight;
			amountLabel.ForeColor = Look.Muted;
			barRow.Controls.Add(bar, 0, 0);
			barRow.Controls.Add(amountLabel, 1, 0);
			AddRow(grid, barRow, SizeType.AutoSize);
			var links = new FlowLayoutPanel { AutoSize = true, WrapContents = false, Margin = new Padding(0, 6, 0, 4) };
			detailsLink.Text = "Show details";
			detailsLink.AutoSize = true;
			detailsLink.LinkClicked += delegate { ToggleDetails(); };
			copyLink.Text = "Copy details";
			copyLink.AutoSize = true;
			copyLink.Visible = false;
			copyLink.Margin = new Padding(16, 3, 3, 3);
			copyLink.LinkClicked += delegate { CopyDetails(); };
			links.Controls.Add(detailsLink);
			links.Controls.Add(copyLink);
			AddRow(grid, links, SizeType.AutoSize);
			logBox.Multiline = true;
			logBox.ReadOnly = true;
			logBox.ScrollBars = ScrollBars.Both;
			logBox.WordWrap = false;
			logBox.Font = new Font("Consolas", 8.5F);
			logBox.BackColor = Look.Paper;
			logBox.Visible = false;
			AddRow(grid, logBox, SizeType.Percent);
			Controls.Add(grid);
			SizeChanged += delegate { statusLabel.MaximumSize = new Size(Math.Max(100, Width - 8), 0); };
			timer.Interval = 100;
			timer.Tick += delegate { Flush(); };
			timer.Start();
		}

		static void AddRow(TableLayoutPanel grid, Control control, SizeType size)
		{
			grid.RowCount++;
			grid.RowStyles.Add(size == SizeType.Percent ? new RowStyle(SizeType.Percent, 100) : new RowStyle(size));
			if (size == SizeType.Percent)
				control.Dock = DockStyle.Fill;
			grid.Controls.Add(control, 0, grid.RowCount - 1);
		}

		public bool Busy { get { return worker != null; } }

		public void Start(string logPath, Action<IReport> work)
		{
			cancelRequested = false;
			failedStep = -1;
			step = -1;
			status = "Starting";
			Interlocked.Exchange(ref done, 0);
			Interlocked.Exchange(ref total, 0);
			statusLabel.ForeColor = Look.Ink;
			copyLink.Visible = false;
			copyLink.Text = "Copy details";
			lock (recent)
				recent.Clear();
			logBox.Clear();
			lock (fileLock)
			{
				try
				{
					Directory.CreateDirectory(Path.GetDirectoryName(logPath));
					if (File.Exists(logPath) && new FileInfo(logPath).Length > 8 << 20)
						File.Delete(logPath);
					file = new StreamWriter(logPath, true, new UTF8Encoding(false)) { AutoFlush = true };
				}
				catch (Exception)
				{
					file = null;
				}
			}
			Log("---- " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + ", launcher " + Assembly.GetExecutingAssembly().GetName().Version);
			worker = new Thread(delegate()
			{
				string failure = null;
				try
				{
					work(this);
				}
				catch (CancelledError)
				{
					failure = "Cancelled.";
				}
				catch (UserError error)
				{
					failure = error.Message;
				}
				catch (Exception error)
				{
					failure = "Something went wrong: " + error.Message;
					Log(error.ToString());
				}
				try
				{
					BeginInvoke(new Action(delegate { Done(failure); }));
				}
				catch (InvalidOperationException)
				{
				}
			});
			worker.IsBackground = true;
			worker.Start();
			Flush();
		}

		public void Cancel()
		{
			cancelRequested = true;
		}

		public void WaitForExit(int milliseconds)
		{
			Thread running = worker;
			if (running != null)
				running.Join(milliseconds);
		}

		void Done(string failure)
		{
			if (failure != null)
				Log("!! " + failure);
			lock (fileLock)
			{
				if (file != null)
					file.Dispose();
				file = null;
			}
			worker = null;
			Flush();
			bar.Style = ProgressBarStyle.Continuous;
			amountLabel.Text = "";
			if (failure == null)
			{
				bar.Value = bar.Maximum;
				step = StepNames.Length;
			}
			else
			{
				failedStep = failure == "Cancelled." ? -1 : step;
				bar.Value = 0;
				statusLabel.Text = failure;
				statusLabel.ForeColor = failure == "Cancelled." ? Look.Ink : Look.Bad;
				copyLink.Visible = true;
			}
			PaintSteps();
			if (Finished != null)
				Finished(failure);
		}

		/* IReport, called from the job's thread */

		public void Log(string line)
		{
			pending.Enqueue(line);
			lock (recent)
			{
				recent.AddLast(line);
				if (recent.Count > 400)
					recent.RemoveFirst();
			}
			lock (fileLock)
			{
				if (file != null)
					file.WriteLine(line);
			}
		}

		public void Status(string text)
		{
			Log("== " + text);
			status = text;
			Progress(0, 0);
		}

		public void Progress(long doneNow, long totalNow)
		{
			Interlocked.Exchange(ref done, doneNow);
			Interlocked.Exchange(ref total, totalNow);
		}

		public void Step(InstallStep now)
		{
			step = (int)now;
		}

		public bool IsCancelled { get { return cancelRequested; } }

		public bool Ask(string question)
		{
			// (after Stop, the window may be waiting for this thread: no question)
			if (cancelRequested)
				throw new CancelledError();
			return (bool)Invoke(new Func<bool>(delegate
			{
				return MessageBox.Show(FindForm(), question, "Halo CE Universal", MessageBoxButtons.OKCancel, MessageBoxIcon.Question) == DialogResult.OK;
			}));
		}

		/* on the window's thread */

		void Flush()
		{
			if (!pending.IsEmpty)
			{
				var text = new StringBuilder();
				string line;
				while (text.Length < 200000 && pending.TryDequeue(out line))
					text.Append(line).Append("\r\n");
				if (logBox.TextLength > 400000)
					logBox.Text = logBox.Text.Substring(logBox.TextLength - 200000);
				logBox.AppendText(text.ToString());
			}
			if (worker == null)
				return;
			if (statusLabel.Text != status)
				statusLabel.Text = status;
			long d = Interlocked.Read(ref done), t = Interlocked.Read(ref total);
			if (t > 0)
			{
				bar.Style = ProgressBarStyle.Continuous;
				bar.Value = (int)Math.Max(0, Math.Min(1000, d * 1000 / t));
				amountLabel.Text = (d * 100 / t) + "%";
			}
			else
			{
				bar.Style = ProgressBarStyle.Marquee;
				amountLabel.Text = d > 0 ? Util.Size(d) : "";
			}
			PaintSteps();
		}

		void PaintSteps()
		{
			if (!withSteps)
				return;
			int current = step;
			for (int i = 0; i < names.Length; i++)
			{
				string mark;
				Color color;
				if (i == failedStep)
				{
					mark = Look.Cross;
					color = Look.Bad;
				}
				else if (i < current)
				{
					mark = Look.Tick;
					color = Look.Good;
				}
				else if (i == current && worker != null)
				{
					mark = Look.Dot;
					color = Look.Busy;
				}
				else
				{
					mark = Look.Ring;
					color = Look.Line;
				}
				if (marks[i].Text != mark)
					marks[i].Text = mark;
				marks[i].ForeColor = color;
				names[i].ForeColor = i <= current || i == failedStep ? Look.Ink : Look.Muted;
				Font font = i == current && worker != null ? currentFont : nameFont;
				if (names[i].Font != font)
					names[i].Font = font;
			}
		}

		void ToggleDetails()
		{
			logBox.Visible = !logBox.Visible;
			detailsLink.Text = logBox.Visible ? "Hide details" : "Show details";
			if (logBox.Visible)
			{
				logBox.SelectionStart = logBox.TextLength;
				logBox.ScrollToCaret();
			}
		}

		void CopyDetails()
		{
			string text;
			lock (recent)
				text = string.Join("\r\n", recent);
			try
			{
				Clipboard.SetText(text);
				copyLink.Text = "Copied";
			}
			catch (ExternalException)
			{
			}
		}

		protected override void Dispose(bool disposing)
		{
			if (disposing)
				timer.Dispose();
			base.Dispose(disposing);
		}
	}

	/* ---------- a file the player supplies */

	sealed class FileCheck
	{
		public bool Ok;
		public bool Near; // the right kind of file, but not one that works (another version)
		public string Message = "";
		public string Input = "";  // what was chosen, or what a search found
		public string Short = "";  // for the summary before installing
		public long Size;          // bytes to copy, if known
	}

	// A file the player drops on the page or chooses, checked in the
	// background; it can also be looked for in the usual folders.
	sealed class FileStep : UserControl
	{
		readonly Func<string, IReport, FileCheck> check;
		readonly string filter;
		readonly string folderDescription;
		readonly Panel zone = new Panel();
		readonly Label zoneLabel = new Label();
		readonly Button chooseButton = new Button();
		readonly LinkLabel folderLink = new LinkLabel();
		readonly Label pathLabel = new Label();
		readonly Label mark = new Label();
		readonly Label message = new Label();
		readonly List<Thread> threads = new List<Thread>();
		volatile int generation;
		volatile bool closing;

		public string Input { get; private set; }
		public bool Ok { get; private set; }
		public string Short { get; private set; }
		public long Bytes { get; private set; }
		public event EventHandler Changed;

		public FileStep(string dropText, string filter, string folderDescription, Func<string, IReport, FileCheck> check)
		{
			this.check = check;
			this.filter = filter;
			this.folderDescription = folderDescription;
			Input = "";
			Short = "";
			BackColor = Look.Paper;
			Size = new Size(540, 190); // room for a message of three lines
			Margin = new Padding(0, 4, 0, 0);

			var grid = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1 };
			grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

			zone.Height = 90;
			zone.Dock = DockStyle.Fill;
			zone.Margin = new Padding(0, 0, 0, 8);
			zone.Paint += delegate(object sender, PaintEventArgs e)
			{
				using (var pen = new Pen(Look.Line, 1.5f) { DashStyle = DashStyle.Dash })
					e.Graphics.DrawRectangle(pen, 1, 1, zone.Width - 3, zone.Height - 3);
			};
			var inside = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, BackColor = Color.Transparent };
			inside.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
			inside.RowStyles.Add(new RowStyle(SizeType.Percent, 50));
			inside.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			inside.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			inside.RowStyles.Add(new RowStyle(SizeType.Percent, 50));
			zoneLabel.Text = dropText;
			zoneLabel.Font = Look.Regular(11F);
			zoneLabel.ForeColor = Look.Muted;
			zoneLabel.AutoSize = true;
			zoneLabel.Anchor = AnchorStyles.None;
			zoneLabel.Margin = new Padding(0, 0, 0, 8);
			var buttons = new FlowLayoutPanel { AutoSize = true, WrapContents = false, Anchor = AnchorStyles.None, BackColor = Color.Transparent };
			chooseButton.Text = "Choose file...";
			chooseButton.AutoSize = true;
			chooseButton.MinimumSize = new Size(120, 32);
			chooseButton.Font = Look.Regular(9.5F);
			chooseButton.Click += delegate { ChooseFile(); };
			folderLink.Text = "or choose a folder";
			folderLink.AutoSize = true;
			folderLink.Margin = new Padding(12, 9, 0, 0);
			folderLink.LinkClicked += delegate { ChooseFolder(); };
			buttons.Controls.Add(chooseButton);
			buttons.Controls.Add(folderLink);
			inside.Controls.Add(zoneLabel, 0, 1);
			inside.Controls.Add(buttons, 0, 2);
			zone.Controls.Add(inside);
			grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			grid.Controls.Add(zone, 0, 0);

			pathLabel.AutoSize = false;
			pathLabel.Dock = DockStyle.Fill;
			pathLabel.AutoEllipsis = true;
			pathLabel.ForeColor = Look.Muted;
			pathLabel.Margin = new Padding(0, 0, 0, 2);
			// (fitted to the text alone, the row cuts off underscores; a minimum
			// size scales with the screen, where a fixed row height would not)
			pathLabel.MinimumSize = new Size(0, 26);
			grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			grid.Controls.Add(pathLabel, 0, 1);

			var state = new TableLayoutPanel { AutoSize = true, Dock = DockStyle.Fill, ColumnCount = 2, Margin = new Padding(0) };
			state.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			state.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
			mark.AutoSize = true;
			mark.Font = Look.Symbols(11F);
			mark.Margin = new Padding(0, 1, 6, 0);
			message.AutoSize = true;
			message.Font = Look.Regular(10F);
			message.MaximumSize = new Size(500, 0);
			state.Controls.Add(mark, 0, 0);
			state.Controls.Add(message, 1, 0);
			grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			grid.Controls.Add(state, 0, 2);
			Controls.Add(grid);

			foreach (Control target in new Control[] { this, zone, inside, zoneLabel })
			{
				target.AllowDrop = true;
				target.DragEnter += OnDragEnter;
				target.DragLeave += delegate { zone.BackColor = Look.Paper; };
				target.DragDrop += OnDragDrop;
			}
		}

		void OnDragEnter(object sender, DragEventArgs e)
		{
			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				e.Effect = DragDropEffects.Copy;
				zone.BackColor = Look.Hover;
			}
		}

		void OnDragDrop(object sender, DragEventArgs e)
		{
			zone.BackColor = Look.Paper;
			string[] dropped = e.Data.GetData(DataFormats.FileDrop) as string[];
			if (dropped != null && dropped.Length > 0)
				SetInput(dropped[0]);
		}

		void ChooseFile()
		{
			using (var dialog = new OpenFileDialog { Filter = filter, CheckFileExists = true })
			{
				if (File.Exists(Input))
					dialog.InitialDirectory = Path.GetDirectoryName(Input);
				if (dialog.ShowDialog(FindForm()) == DialogResult.OK)
					SetInput(dialog.FileName);
			}
		}

		void ChooseFolder()
		{
			using (var dialog = new FolderBrowserDialog { Description = folderDescription, ShowNewFolderButton = false })
			{
				if (Directory.Exists(Input))
					dialog.SelectedPath = Input;
				if (dialog.ShowDialog(FindForm()) == DialogResult.OK)
					SetInput(dialog.SelectedPath);
			}
		}

		public void SetInput(string input)
		{
			input = (input ?? "").Trim().Trim('"');
			Input = input;
			pathLabel.Text = input;
			Run(delegate(IReport report) { return check(input, report); }, "Checking...");
		}

		// looks through folders with find, which returns what it found there
		public void Search(List<string> folders, Func<string, IReport, FileCheck> find, string notFound)
		{
			pathLabel.Text = "";
			Run(delegate(IReport report)
			{
				FileCheck closest = null;
				foreach (string folder in folders)
				{
					FileCheck found = find(folder, report);
					if (found.Ok)
						return found;
					if (closest == null && found.Near)
						closest = found;
				}
				return closest ?? new FileCheck { Message = notFound };
			}, "Looking for it in your Downloads, Desktop and Documents folders...");
		}

		// what is installed already will do
		public void ShowInstalled(string text)
		{
			++generation;
			Input = "";
			pathLabel.Text = "";
			Ok = true;
			Short = "Already installed";
			Bytes = 0;
			SetState(true, text);
			if (Changed != null)
				Changed(this, EventArgs.Empty);
		}

		// nothing chosen yet
		public void Reset()
		{
			++generation;
			Input = "";
			pathLabel.Text = "";
			Ok = false;
			Short = "";
			Bytes = 0;
			mark.Text = "";
			message.Text = "";
			if (Changed != null)
				Changed(this, EventArgs.Empty);
		}

		void Run(Func<IReport, FileCheck> work, string busyText)
		{
			int mine = ++generation;
			Ok = false;
			SetState(null, busyText);
			if (Changed != null)
				Changed(this, EventArgs.Empty);
			// the result comes back through this control's window, which a
			// page that has not been shown yet does not have
			if (!IsHandleCreated)
				CreateHandle();
			var thread = new Thread(delegate()
			{
				FileCheck result;
				try
				{
					result = work(new LogOnlyReport(null, delegate { return closing || mine != generation; }));
				}
				catch (CancelledError)
				{
					return;
				}
				catch (Exception error)
				{
					result = new FileCheck { Message = error.Message };
				}
				try
				{
					BeginInvoke(new Action(delegate
					{
						if (mine == generation)
							Apply(result);
					}));
				}
				catch (InvalidOperationException)
				{
				}
			});
			thread.IsBackground = true;
			lock (threads)
			{
				threads.RemoveAll(t => !t.IsAlive);
				threads.Add(thread);
			}
			thread.Start();
		}

		void Apply(FileCheck result)
		{
			if (result.Input.Length > 0)
			{
				Input = result.Input;
				pathLabel.Text = result.Input;
			}
			Ok = result.Ok;
			Short = result.Short;
			Bytes = result.Size;
			SetState(result.Ok, result.Message);
			if (Changed != null)
				Changed(this, EventArgs.Empty);
		}

		void SetState(bool? ok, string text)
		{
			mark.Text = ok == null ? Look.Dot : ok.Value ? Look.Tick : Look.Cross;
			mark.ForeColor = ok == null ? Look.Busy : ok.Value ? Look.Good : Look.Bad;
			message.Text = text;
			message.ForeColor = ok == false ? Look.Bad : Look.Ink;
		}

		// stops the checks that are running (their tar.exe too)
		public void Stop()
		{
			closing = true;
			Thread[] running;
			lock (threads)
				running = threads.ToArray();
			foreach (Thread thread in running)
				thread.Join(2000);
		}
	}

	/* ---------- the setup window */

	// The first run, step by step in plain words. Started from the launcher
	// (changing), it replaces the game files.
	sealed class SetupWizard : Form
	{
		const string Title = "Halo CE Universal Setup";
		const int TextWidth = 540;
		const int Welcome = 0, GamePage = 1, Ready = 2, Installing = 3, Finished = 4;
		static readonly string[] Steps = { "Welcome", "Halo game", "Install", "Done" };
		static readonly int[] StepOfPage = { 0, 1, 2, 2, 3 };

		Settings settings;
		readonly bool changing;
		readonly Panel content = new Panel();
		readonly Label[] stepMarks = new Label[Steps.Length];
		readonly Label[] stepNames = new Label[Steps.Length];
		readonly Button backButton = new Button();
		readonly Button nextButton = new Button();
		readonly Button cancelButton = new Button();
		readonly Control[] pages = new Control[5];
		readonly FileStep gameStep;
		readonly ProgressPanel progress = new ProgressPanel(true);
		readonly Label installHeading;
		readonly Label installText;
		readonly Button retryButton = new Button();
		readonly Label readyGame = new Label();
		readonly Label readyFolder = new Label();
		readonly Label readySpace = new Label();
		readonly LinkLabel changeFolder = new LinkLabel();
		readonly Panel toolsNote = new Panel();
		int page = -1;
		bool installed;
		bool spaceOk = true;
		bool quitConfirmed;
		volatile int buildTools; // 0 not known yet, 1 installed, 2 missing

		// Finish was chosen: open the launcher next
		public bool OpenLauncher;
		public string Root { get { return settings.Root; } }

		public SetupWizard(Settings settings, bool changing)
		{
			this.settings = settings;
			this.changing = changing;
			gameStep = new FileStep("Drag your Halo game here",
				"Halo disc image or archive|*.iso;*.xiso;*.zip;*.7z;*.rar|Halo map files|*.map|All files|*.*",
				"Choose the folder with Halo's files (the one that has a folder called maps in it).",
				CheckGame);
			installHeading = Heading("Installing Halo");
			installText = Paragraph("");
			Build();
			gameStep.Changed += delegate { UpdateButtons(); };
			progress.Finished += InstallFinished;
			Shown += delegate
			{
				var probe = new Thread(delegate()
				{
					buildTools = Toolchain.HasBuildTools() ? 1 : 2;
					try
					{
						BeginInvoke(new Action(delegate { if (page == Ready) FillReady(); }));
					}
					catch (InvalidOperationException)
					{
					}
				});
				probe.IsBackground = true;
				probe.Start();
				Prefill();
				ShowPage(changing ? GamePage : Welcome);
				// (opened from the launcher, the launcher offers it)
				if (!changing)
					SelfUpdate.Offer(this, delegate { return progress.Busy; });
			};
			FormClosing += OnClosing;
		}

		/* layout */

		static Label Heading(string text)
		{
			return new Label { Text = text, Font = Look.Bold(17F), ForeColor = Look.Ink, AutoSize = true, MaximumSize = new Size(TextWidth, 0), Margin = new Padding(0, 0, 0, 14) };
		}

		static Label Paragraph(string text)
		{
			return Look.Text(text, 10.5F, Look.Ink, TextWidth);
		}

		static Label Small(string text)
		{
			return Look.Text(text, 9.5F, Look.Muted, TextWidth);
		}

		// what to look for, in big letters
		static Label Big(string text, int spaceAfter = 2)
		{
			return new Label { Text = text, Font = Look.Bold(13F), ForeColor = Look.Ink, AutoSize = true, MaximumSize = new Size(TextWidth, 0), Margin = new Padding(0, 0, 0, spaceAfter) };
		}

		static FlowLayoutPanel Page(params Control[] controls)
		{
			var flow = new FlowLayoutPanel
			{
				Dock = DockStyle.Fill,
				FlowDirection = FlowDirection.TopDown,
				WrapContents = false,
				AutoScroll = true,
				BackColor = Look.Paper,
			};
			flow.Controls.AddRange(controls);
			return flow;
		}

		void Build()
		{
			SuspendLayout();
			AutoScaleDimensions = new SizeF(96F, 96F);
			AutoScaleMode = AutoScaleMode.Dpi;
			Font = Look.Regular(9.5F);
			Text = Title;
			Icon = AppIcon.Window;
			FormBorderStyle = FormBorderStyle.FixedSingle;
			MaximizeBox = false;
			StartPosition = FormStartPosition.CenterScreen;
			ClientSize = new Size(840, 620);
			BackColor = Look.Paper;

			// the side: what this is, and where the player is
			var side = new Panel { Dock = DockStyle.Left, Width = 230, BackColor = Look.Dark };
			side.Controls.Add(new PictureBox { Image = AppIcon.Draw(128), SizeMode = PictureBoxSizeMode.Zoom, Size = new Size(56, 56), Location = new Point(24, 28) });
			side.Controls.Add(new Label { Text = "Halo CE Universal", Font = Look.Bold(12.5F), ForeColor = Look.OnDark, AutoSize = true, Location = new Point(24, 96) });
			side.Controls.Add(new Label { Text = changing ? "Change your files" : "Setup", Font = Look.Regular(10F), ForeColor = Look.OnDarkMuted, AutoSize = true, Location = new Point(25, 122) });
			for (int i = 0; i < Steps.Length; i++)
			{
				stepMarks[i] = new Label { Font = Look.Symbols(10F), AutoSize = true, Location = new Point(24, 176 + i * 34) };
				stepNames[i] = new Label { Text = Steps[i], Font = Look.Regular(10.5F), AutoSize = true, Location = new Point(48, 174 + i * 34) };
				side.Controls.Add(stepMarks[i]);
				side.Controls.Add(stepNames[i]);
			}

			// the buttons
			var bar = new Panel { Dock = DockStyle.Bottom, Height = 64, BackColor = Look.Bar };
			var buttons = new FlowLayoutPanel { Dock = DockStyle.Right, FlowDirection = FlowDirection.RightToLeft, AutoSize = true, WrapContents = false, Padding = new Padding(0, 14, 20, 0), BackColor = Look.Bar };
			foreach (Button button in new[] { cancelButton, nextButton, backButton })
			{
				button.AutoSize = true;
				button.MinimumSize = new Size(104, 36);
				button.Font = Look.Regular(10F);
				button.Margin = new Padding(8, 0, 0, 0);
				buttons.Controls.Add(button);
			}
			backButton.Text = "< Back";
			cancelButton.Text = "Cancel";
			backButton.Click += delegate { Back(); };
			nextButton.Click += delegate { Next(); };
			cancelButton.Click += delegate { if (progress.Busy) progress.Cancel(); else Close(); };
			bar.Controls.Add(buttons);

			content.Dock = DockStyle.Fill;
			content.Padding = new Padding(36, 32, 24, 8);
			content.BackColor = Look.Paper;

			// Every page is in the window from the start, hidden until shown:
			// so all of them are scaled for the screen, and a check that
			// finishes before its page is shown can still report.
			BuildPages();
			foreach (Control each in pages)
			{
				each.Visible = false;
				content.Controls.Add(each);
			}
			Controls.Add(content);
			Controls.Add(bar);
			Controls.Add(side);
			ResumeLayout(true);
		}

		void BuildPages()
		{
			var game = Paragraph("Halo: Combat Evolved for the original Xbox, American or European version");
			game.Font = Look.Bold(10.5F);
			game.Margin = new Padding(0, 4, 0, 2);
			var gameHint = Small("Your own copy of the game, copied from the disc to your PC, usually as a disc image (.iso).");
			gameHint.Margin = new Padding(0, 0, 0, 12);
			pages[Welcome] = Page(
				Heading("Let's get Halo running on your PC"),
				Paragraph("This sets up Halo: Combat Evolved, the original Xbox game, rebuilt from its source code to run on Windows."),
				Paragraph("You need one thing that can't be downloaded for you:"),
				game, gameHint,
				Paragraph("Everything else is downloaded for you. Setting up takes about 5 to 10 minutes."),
				Small("The game is built from " + Pinned.RepositoryUrl + ". Your files stay on this PC."));

			pages[GamePage] = Page(
				Heading("Step 1 of 2: Your Halo game"),
				Paragraph("Your copy of Halo: Combat Evolved for the original Xbox, the American (NTSC) or European (PAL) version, copied to your PC. " +
					"That's usually a disc image ending in .iso, or a folder with the game's files (it has a folder called maps)."),
				Big("Halo - Combat Evolved (USA) (Rev 1).xiso.iso"),
				Big("GOOGLE IS YOUR FRIEND"),
				Big("Halo - Combat Evolved (USA) (Rev 2).xiso.iso"),
				Big("GOOGLE IS YOUR FRIEND"),
				Big("Halo - Combat Evolved (USA).xiso.iso"),
				Big("GOOGLE IS YOUR FRIEND"),
				Big("WE LOVE ARCHIVE.ORG", 10),
				gameStep);

			var summary = new TableLayoutPanel { AutoSize = true, ColumnCount = 2, Margin = new Padding(0, 0, 0, 12) };
			summary.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			summary.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			AddSummary(summary, "Halo game", readyGame);
			AddSummary(summary, "Install to", readyFolder);
			changeFolder.Text = "Change...";
			changeFolder.AutoSize = true;
			changeFolder.Margin = new Padding(0, 0, 0, 10);
			changeFolder.LinkClicked += delegate { ChangeFolder(); };
			summary.RowCount++;
			summary.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			summary.Controls.Add(changeFolder, 1, summary.RowCount - 1);
			AddSummary(summary, "Space", readySpace);
			toolsNote.BackColor = Look.Note;
			toolsNote.Size = new Size(TextWidth, 70);
			toolsNote.Padding = new Padding(12, 10, 12, 10);
			var note = Look.Text("Your PC also needs Microsoft's free C++ build tools (" + Pinned.BuildToolsSize + " on " +
				Path.GetPathRoot(Util.ProgramFilesX86).TrimEnd('\\') + "). Setup installs them for you: when Windows asks for permission, click Yes.",
				10F, Look.Ink, TextWidth - 24);
			note.Dock = DockStyle.Fill;
			toolsNote.Controls.Add(note);
			toolsNote.Visible = false;
			pages[Ready] = Page(
				Heading(changing ? "Ready to rebuild Halo" : "Step 2 of 2: Ready to install"),
				summary,
				toolsNote);

			progress.Size = new Size(TextWidth, 300);
			retryButton.Text = "Try again";
			retryButton.AutoSize = true;
			retryButton.MinimumSize = new Size(120, 34);
			retryButton.Visible = false;
			retryButton.Click += delegate { StartInstall(); };
			pages[Installing] = Page(installHeading, installText, progress, retryButton);

			var play = Look.Primary("Play Halo", 14F);
			play.Size = new Size(240, 56);
			play.Margin = new Padding(0, 6, 0, 18);
			play.Click += delegate { PlayNow(); };
			// (the launcher that opened this window has its own Play)
			play.Visible = !changing;
			var controls = new LinkLabel { Text = "See all the controls", AutoSize = true, Margin = new Padding(0, 0, 0, 0) };
			controls.LinkClicked += delegate { Help.ShowControls(this); };
			pages[Finished] = Page(
				Heading(changing ? "Done! Halo was rebuilt with your files" : "Halo is ready!"),
				Paragraph(changing ? "Finish takes you back to the launcher." :
					"Start it any time with Halo CE Universal on your desktop or in the Start menu."),
				play,
				Small("Good to know: Esc opens the game's menu, F12 frees the mouse, and a gamepad works too."),
				controls);
		}

		static void AddSummary(TableLayoutPanel summary, string caption, Label value)
		{
			summary.RowCount++;
			summary.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			var label = new Label { Text = caption, Font = Look.Regular(10.5F), ForeColor = Look.Muted, AutoSize = true, Margin = new Padding(0, 0, 18, 10) };
			value.Font = Look.Regular(10.5F);
			value.ForeColor = Look.Ink;
			value.AutoSize = true;
			value.MaximumSize = new Size(360, 0);
			value.Margin = new Padding(0, 0, 0, 10);
			summary.Controls.Add(label, 0, summary.RowCount - 1);
			summary.Controls.Add(value, 1, summary.RowCount - 1);
		}

		/* the player's files */

		static FileCheck CheckGame(string input, IReport report)
		{
			using (GameDataSource source = GameData.Open(input, Path.GetTempPath(), report, false))
			{
				if (source == null)
				{
					return new FileCheck
					{
						Message = File.Exists(input) || Directory.Exists(input)
							? "No Halo game files were found here. Choose the disc image (.iso), or the folder that has a folder called maps in it."
							: "That file or folder doesn't exist.",
					};
				}
				if (source is ArchiveSource)
					return new FileCheck { Ok = true, Message = source.Description, Input = input, Short = Path.GetFileName(input) };
				string problem = GameData.Problem(source.Maps);
				bool halo = source.Maps.Count > 0 && source.Maps.All(m => m.Valid);
				return new FileCheck
				{
					Ok = problem == null,
					Near = problem != null && halo,
					Message = problem ?? source.Friendly(),
					Input = problem == null || halo ? source.Location : "",
					Short = source.Maps.Count + " maps, " + GameData.VersionName(source.Build) + " (" + Util.Size(source.TotalBytes) + ")",
					Size = source.TotalBytes,
				};
			}
		}

		// For the install folder: what is installed there already, else what
		// was chosen in this window, else what was saved, else nothing (and a
		// search when the page is shown).
		void Prefill()
		{
			Folders folders = new Folders(settings.Root);
			if (GameData.IsInstalled(folders.Data))
				gameStep.ShowInstalled("Already installed. Drag other game files here only to replace them.");
			else if (gameStep.Input.Length == 0)
			{
				if (settings.DataInput.Length > 0)
					gameStep.SetInput(settings.DataInput);
				else
				{
					gameStep.Reset();
					searchedGame = false;
				}
			}
		}

		bool searchedGame;
		DateTime pageShown;

		void SearchIfNeeded()
		{
			List<string> usual = KnownFolders.Usual();
			if (page == GamePage && !searchedGame && !gameStep.Ok && gameStep.Input.Length == 0)
			{
				searchedGame = true;
				gameStep.Search(usual, CheckGame, "We couldn't find it on this PC by ourselves. Drag it here, or click Choose file.");
			}
		}

		/* moving between pages */

		void ShowPage(int index)
		{
			page = index;
			pageShown = DateTime.UtcNow;
			for (int i = 0; i < pages.Length; i++)
				pages[i].Visible = i == index;
			int current = StepOfPage[index];
			for (int i = 0; i < Steps.Length; i++)
			{
				bool doneStep = i < current || (installed && i <= current);
				stepMarks[i].Text = doneStep ? Look.Tick : i == current ? Look.Dot : Look.Ring;
				stepMarks[i].ForeColor = doneStep ? Color.FromArgb(0x6F, 0xD0, 0x8C) : i == current ? Look.OnDark : Look.OnDarkMuted;
				stepNames[i].ForeColor = i == current ? Look.OnDark : Look.OnDarkMuted;
				stepNames[i].Font = i == current ? Look.Bold(10.5F) : Look.Regular(10.5F);
			}
			if (index == Ready)
				FillReady();
			SearchIfNeeded();
			UpdateButtons();
		}

		void UpdateButtons()
		{
			bool busy = progress.Busy;
			backButton.Visible = (page > (changing ? GamePage : Welcome) && page <= Ready) || (page == Installing && !busy);
			nextButton.Visible = page != Installing;
			nextButton.Text = page == Ready ? (changing ? "Rebuild" : "Install") : page == Finished ? "Finish" : "Next >";
			nextButton.Enabled = page == Welcome || page == Finished || (page == GamePage && gameStep.Ok) ||
				(page == Ready && spaceOk && gameStep.Ok);
			cancelButton.Visible = page != Finished;
			cancelButton.Text = busy ? "Stop" : "Cancel";
			AcceptButton = nextButton.Visible && nextButton.Enabled ? nextButton : null;
		}

		// A double-click on Next or Back is two clicks: the second, on the
		// page that the first one opened, is not meant for it.
		bool JustShown()
		{
			return (DateTime.UtcNow - pageShown).TotalMilliseconds < SystemInformation.DoubleClickTime;
		}

		void Back()
		{
			if (JustShown())
				return;
			if (page == Installing)
				ShowPage(Ready);
			else if (page > Welcome)
				ShowPage(page - 1);
		}

		void Next()
		{
			if (JustShown())
				return;
			if (page == Ready)
				StartInstall();
			else if (page == Finished)
			{
				OpenLauncher = !changing;
				Close();
			}
			else
				ShowPage(page + 1);
		}

		void FillReady()
		{
			SetSummary(readyGame, gameStep);
			readyFolder.Text = settings.Root;
			changeFolder.Visible = !changing;
			long needed = (1500L << 20) + (gameStep.Bytes > 0 ? gameStep.Bytes : gameStep.Input.Length > 0 ? 2048L << 20 : 0);
			// Microsoft's build tools go on the Windows drive
			string windowsDrive = Path.GetPathRoot(Util.ProgramFilesX86).TrimEnd('\\');
			bool toolsHere = buildTools == 2 && Util.SameDrive(settings.Root, Util.ProgramFilesX86);
			if (toolsHere)
				needed += Pinned.BuildToolsSpace;
			long free = Util.FreeSpace(settings.Root);
			spaceOk = free < 0 || free >= needed;
			string text = "About " + Util.Size(needed) + " needed" + (toolsHere ? " with Microsoft's build tools" : "") +
				(free >= 0 ? ", " + Util.Size(free) + " free" : "");
			string advice = spaceOk ? "" : toolsHere ? ". Free up some space on " + windowsDrive : ". Free up some space, or install somewhere else";
			if (buildTools == 2 && !toolsHere)
			{
				long windowsFree = Util.FreeSpace(Util.ProgramFilesX86);
				bool toolsOk = windowsFree < 0 || windowsFree >= Pinned.BuildToolsSpace;
				text += "; and " + Util.Size(Pinned.BuildToolsSpace) + " on " + windowsDrive + " for Microsoft's build tools" +
					(windowsFree >= 0 ? ", " + Util.Size(windowsFree) + " free" : "");
				if (!toolsOk)
					advice += (advice.Length > 0 ? "; and free up some space on " : ". Free up some space on ") + windowsDrive;
				spaceOk = spaceOk && toolsOk;
			}
			readySpace.Text = text + (spaceOk ? "" : advice + ".");
			readySpace.ForeColor = spaceOk ? Look.Ink : Look.Bad;
			toolsNote.Visible = buildTools == 2;
			UpdateButtons();
		}

		static void SetSummary(Label label, FileStep step)
		{
			if (step.Ok)
			{
				label.Text = step.Short.Length > 0 ? step.Short : "Chosen";
				label.ForeColor = Look.Ink;
			}
			else
			{
				label.Text = "Not chosen yet: go Back to choose it";
				label.ForeColor = Look.Bad;
			}
		}

		void ChangeFolder()
		{
			using (var dialog = new FolderBrowserDialog { Description = "Choose where to install Halo CE Universal.", ShowNewFolderButton = true })
			{
				if (dialog.ShowDialog(this) != DialogResult.OK)
					return;
				string chosen = Folders.IsOwn(dialog.SelectedPath) ? dialog.SelectedPath : Path.Combine(dialog.SelectedPath, "HaloCEUniversal");
				string root;
				string problem = Folders.Problem(chosen, out root);
				if (problem != null)
				{
					MessageBox.Show(this, problem, Title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
					return;
				}
				if (root.Any(c => c > 127) && MessageBox.Show(this, "Halo may not find its files in a folder whose name has letters " +
					"outside your Windows language's alphabet. Install into " + root + " anyway?", Title, MessageBoxButtons.YesNo,
					MessageBoxIcon.Warning) != DialogResult.Yes)
					return;
				UseFolder(root);
			}
		}

		// An install there keeps its own settings (the files chosen in this
		// window still count); a new folder takes the ones so far. What was
		// installed in the old folder is not in the new one.
		void UseFolder(string root)
		{
			Settings next = Settings.ForRoot(root);
			if (next.Exists)
			{
				if (gameStep.Input.Length > 0)
					next.DataInput = gameStep.Input;
			}
			else
			{
				next = settings;
				next.Root = root;
				next.Commit = "";
				next.InstalledData = "";
			}
			settings = next;
			Prefill();
			FillReady();
		}

		/* installing */

		void StartInstall()
		{
			settings.DataInput = gameStep.Input;
			string root;
			string problem = Folders.Problem(settings.Root, out root);
			if (problem != null)
			{
				MessageBox.Show(this, problem, Title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
				return;
			}
			settings.Root = root;
			ShowPage(Installing);
			installHeading.Text = changing ? "Rebuilding Halo" : "Installing Halo";
			installText.Text = "This takes a few minutes. You can keep using your PC in the meantime.";
			retryButton.Visible = false;
			Settings chosen = settings;
			bool fresh = !changing;
			progress.Start(new Folders(root).LauncherLog, delegate(IReport report) { Installer.Run(chosen, report, fresh, true); });
			UpdateButtons();
		}

		void InstallFinished(string failure)
		{
			if (failure == null)
			{
				installed = true;
				ShowPage(Finished);
				return;
			}
			bool stopped = failure == "Cancelled.";
			installHeading.Text = stopped ? "Setup was stopped" : "Something went wrong";
			installText.Text = stopped
				? "Nothing is lost: Try again continues where it stopped."
				: "Try again, or go Back to choose other files. Copy details copies what happened, for asking for help.";
			retryButton.Visible = true;
			UpdateButtons();
		}

		void PlayNow()
		{
			try
			{
				GameLauncher.Start(new Folders(settings.Root), settings).Dispose();
			}
			catch (Exception error)
			{
				MessageBox.Show(this, "Couldn't start Halo: " + error.Message, Title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
				return;
			}
			Close();
		}

		void OnClosing(object sender, FormClosingEventArgs e)
		{
			if (progress.Busy)
			{
				if (MessageBox.Show(this, "Stop setting up Halo? You can start setup again later; it continues where it stopped.",
					Title, MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
				{
					e.Cancel = true;
					return;
				}
				progress.Cancel();
				progress.WaitForExit(8000);
			}
			else if (!installed && !changing && page > Welcome && !quitConfirmed && e.CloseReason == CloseReason.UserClosing)
			{
				if (MessageBox.Show(this, "Quit setup? Halo isn't installed yet.", Title, MessageBoxButtons.YesNo,
					MessageBoxIcon.Question) != DialogResult.Yes)
				{
					e.Cancel = true;
					return;
				}
				quitConfirmed = true;
			}
			gameStep.Stop();
		}
	}

	/* ---------- the launcher */

	// After setup: Play, updates and settings; the rest is under More.
	sealed class MainForm : Form
	{
		const string Title = "Halo CE Universal";

		Settings settings;
		readonly Label stateLabel = new Label();
		readonly Button playButton = Look.Primary("Play", 18F);
		readonly Label updateLabel = new Label();
		readonly Button updateButton = new Button();
		readonly Button moreButton = new Button();
		readonly ContextMenuStrip moreMenu = new ContextMenuStrip();
		readonly ToolStripMenuItem developerItem = new ToolStripMenuItem("Developer build (stops at the first failed assertion)");
		readonly List<ToolStripItem> busyItems = new List<ToolStripItem>();
		readonly ProgressPanel progress = new ProgressPanel(false);
		Process game;
		Folders gameFolders;
		string latestCommit;
		bool checking;

		public MainForm(Settings settings)
		{
			this.settings = settings;
			Build();
			progress.Finished += WorkFinished;
			Shown += delegate
			{
				RefreshState();
				ActiveControl = playButton;
				CheckForUpdate(false);
				SelfUpdate.Offer(this, delegate { return progress.Busy || game != null; });
			};
			FormClosing += OnClosing;
		}

		Folders CurrentFolders { get { return new Folders(settings.Root); } }

		void Build()
		{
			SuspendLayout();
			AutoScaleDimensions = new SizeF(96F, 96F);
			AutoScaleMode = AutoScaleMode.Dpi;
			Font = Look.Regular(9.5F);
			Text = Title;
			Icon = AppIcon.Window;
			FormBorderStyle = FormBorderStyle.FixedSingle;
			MaximizeBox = false;
			StartPosition = FormStartPosition.CenterScreen;
			ClientSize = new Size(640, 380);
			BackColor = Look.Paper;

			var header = new Panel { Dock = DockStyle.Top, Height = 108, BackColor = Look.Dark };
			header.Controls.Add(new PictureBox { Image = AppIcon.Draw(128), SizeMode = PictureBoxSizeMode.Zoom, Size = new Size(64, 64), Location = new Point(28, 22) });
			header.Controls.Add(new Label { Text = "Halo: Combat Evolved", Font = Look.Bold(18F), ForeColor = Look.OnDark, AutoSize = true, Location = new Point(106, 22) });
			stateLabel.Font = Look.Regular(10.5F);
			stateLabel.ForeColor = Look.OnDarkMuted;
			stateLabel.AutoSize = true;
			stateLabel.Location = new Point(109, 62);
			header.Controls.Add(stateLabel);

			var body = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, Padding = new Padding(28, 26, 28, 14), BackColor = Look.Paper };
			body.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
			playButton.Size = new Size(300, 66);
			playButton.Anchor = AnchorStyles.None;
			playButton.Margin = new Padding(0, 0, 0, 18);
			playButton.Click += delegate { Play(); };
			AddRow(body, playButton, SizeType.AutoSize);

			var updateRow = new FlowLayoutPanel { AutoSize = true, Anchor = AnchorStyles.None, WrapContents = false, Margin = new Padding(0, 0, 0, 16) };
			updateLabel.AutoSize = true;
			updateLabel.Font = Look.Regular(10F);
			updateLabel.Margin = new Padding(0, 8, 10, 0);
			updateButton.AutoSize = true;
			updateButton.MinimumSize = new Size(150, 34);
			updateButton.Font = Look.Regular(9.5F);
			updateButton.Click += delegate { UpdateClicked(); };
			updateRow.Controls.Add(updateLabel);
			updateRow.Controls.Add(updateButton);
			AddRow(body, updateRow, SizeType.AutoSize);

			var buttons = new FlowLayoutPanel { AutoSize = true, Anchor = AnchorStyles.None, WrapContents = false, Margin = new Padding(0, 0, 0, 12) };
			Button settingsButton = Look.Plain("Settings");
			settingsButton.Click += delegate { ShowSettings(); };
			Button controlsButton = Look.Plain("Controls");
			controlsButton.Click += delegate { Help.ShowControls(this); };
			Button helpButton = Look.Plain("Help");
			helpButton.Click += delegate { Help.Show(this, CurrentFolders); };
			moreButton.Text = "More \u25BE";
			moreButton.AutoSize = true;
			moreButton.MinimumSize = new Size(110, 34);
			moreButton.Font = Look.Regular(9.5F);
			moreButton.Margin = new Padding(4, 0, 4, 0);
			moreButton.Click += delegate { moreMenu.Show(moreButton, new Point(0, moreButton.Height)); };
			buttons.Controls.AddRange(new Control[] { settingsButton, controlsButton, helpButton, moreButton });
			AddRow(body, buttons, SizeType.AutoSize);

			progress.Visible = false;
			AddRow(body, progress, SizeType.Percent);

			AddItem("Repair Halo", delegate { RunWork(false); }, true);
			AddItem("Use other game files...", delegate { ChangeFiles(); }, true);
			AddItem("Open the install folder", delegate { OpenFolder(); }, false);
			AddItem("Open the log", delegate { OpenLog(); }, false);
			developerItem.Click += delegate { ToggleDeveloper(); };
			moreMenu.Items.Add(developerItem);
			busyItems.Add(developerItem);
			moreMenu.Items.Add(new ToolStripSeparator());
			AddItem("Uninstall Halo CE Universal...", delegate { Uninstall(); }, true);

			Controls.Add(body);
			Controls.Add(header);
			ResumeLayout(true);
		}

		static void AddRow(TableLayoutPanel grid, Control control, SizeType size)
		{
			grid.RowCount++;
			grid.RowStyles.Add(size == SizeType.Percent ? new RowStyle(SizeType.Percent, 100) : new RowStyle(size));
			if (size == SizeType.Percent)
				control.Dock = DockStyle.Fill;
			grid.Controls.Add(control, 0, grid.RowCount - 1);
		}

		void AddItem(string text, Action action, bool notWhileBusy)
		{
			var item = new ToolStripMenuItem(text);
			item.Click += delegate { action(); };
			moreMenu.Items.Add(item);
			if (notWhileBusy)
				busyItems.Add(item);
		}

		void RefreshState()
		{
			Folders folders = CurrentFolders;
			bool busy = progress.Busy;
			bool running = game != null;
			bool built = folders.GameBuilt;
			bool data = built && GameData.IsInstalled(folders.Data);
			bool behind = latestCommit != null && settings.Commit.Length > 0 && latestCommit != settings.Commit;
			stateLabel.Text = running ? "Halo is running" : busy ? "Working..." : !built ? "Halo needs to be built again: More > Repair Halo"
				: !data ? "Your game files are missing" : behind ? "An update is available" : "Ready to play";
			playButton.Enabled = !busy && !running && built;
			playButton.Text = running ? "Running..." : built && !data ? "Fix game files" : "Play";
			updateButton.Enabled = !busy && !running && !checking && built;
			updateButton.Text = behind ? "Update now" : "Check for updates";
			if (checking)
			{
				updateLabel.Text = "Checking for updates...";
				updateLabel.ForeColor = Look.Muted;
			}
			else if (behind)
			{
				updateLabel.Text = "An update is available.";
				updateLabel.ForeColor = Look.Ink;
			}
			else if (latestCommit != null && latestCommit == settings.Commit)
			{
				updateLabel.Text = "You have the latest version.";
				updateLabel.ForeColor = Look.Good;
			}
			else
			{
				updateLabel.Text = "";
			}
			foreach (ToolStripItem item in busyItems)
				item.Enabled = !busy && !running;
			developerItem.Checked = settings.DeveloperBuild;
		}

		/* updates */

		void CheckForUpdate(bool asked)
		{
			if (checking || !CurrentFolders.GameBuilt)
				return;
			checking = true;
			RefreshState();
			var thread = new Thread(delegate()
			{
				string latest = null;
				try
				{
					latest = SourceTree.LatestCommit();
				}
				catch (Exception)
				{
				}
				try
				{
					BeginInvoke(new Action(delegate
					{
						checking = false;
						if (latest != null)
							latestCommit = latest;
						else if (asked)
							MessageBox.Show(this, "Couldn't check for updates. Check your internet connection, then try again.", Title, MessageBoxButtons.OK, MessageBoxIcon.Information);
						RefreshState();
					}));
				}
				catch (InvalidOperationException)
				{
				}
			});
			thread.IsBackground = true;
			thread.Start();
		}

		void UpdateClicked()
		{
			if (latestCommit != null && settings.Commit.Length > 0 && latestCommit != settings.Commit)
				RunWork(true);
			else
				CheckForUpdate(true);
		}

		// Updates (with update) or repairs, showing the progress below.
		void RunWork(bool update)
		{
			if (progress.Busy || game != null)
				return;
			Folders folders = CurrentFolders;
			Settings chosen = settings;
			bool updateSource = update || !File.Exists(Path.Combine(folders.Game, "configure.py"));
			ShowProgress(true);
			progress.Start(folders.LauncherLog, delegate(IReport report) { Installer.Run(chosen, report, updateSource, true); });
			RefreshState();
		}

		void WorkFinished(string failure)
		{
			// what went wrong stays on show, with Copy details
			if (failure == null)
			{
				ShowProgress(false);
				// there may be something newer than what the last check saw
				latestCommit = null;
				CheckForUpdate(false);
			}
			RefreshState();
		}

		// the window grows to show the progress, and shrinks back after
		void ShowProgress(bool show)
		{
			progress.Visible = show;
			ClientSize = new Size(ClientSize.Width, LogicalToDevice(show ? 560 : 380));
		}

		int LogicalToDevice(int value)
		{
			return (int)Math.Round(value * CurrentAutoScaleDimensions.Height / 96F);
		}

		/* playing */

		void Play()
		{
			Folders folders = CurrentFolders;
			if (!folders.GameBuilt || game != null)
				return;
			if (!GameData.IsInstalled(folders.Data))
			{
				ChangeFiles();
				return;
			}
			try
			{
				game = GameLauncher.Start(folders, settings);
			}
			catch (Exception error)
			{
				MessageBox.Show(this, "Couldn't start Halo: " + error.Message, Title, MessageBoxButtons.OK, MessageBoxIcon.Warning);
				return;
			}
			gameFolders = folders;
			// the handler first: a game that stops at once raises Exited as
			// soon as events are enabled
			game.Exited += delegate
			{
				try
				{
					BeginInvoke(new Action(GameExited));
				}
				catch (InvalidOperationException)
				{
				}
			};
			game.EnableRaisingEvents = true;
			RefreshState();
		}

		void GameExited()
		{
			if (game == null)
				return;
			int code = game.ExitCode;
			game.Dispose();
			game = null;
			RefreshState();
			if (code == 0)
				return;
			string log = gameFolders.GameLog;
			if (MessageBox.Show(this, "Halo stopped because of a problem (code " + code + "). The end of its log:\r\n\r\n" + Util.Tail(log, 20) +
				"\r\n\r\nOpen the whole log?", Title, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
			{
				Process.Start("notepad.exe", Util.Quote(log));
			}
		}

		/* the rest */

		void ShowSettings()
		{
			using (var dialog = new GameSettingsForm(settings))
			{
				if (dialog.ShowDialog(this) == DialogResult.OK && settings.Exists)
					settings.Save();
			}
		}

		void ChangeFiles()
		{
			using (var wizard = new SetupWizard(settings, true))
				wizard.ShowDialog(this);
			settings = Settings.ForRoot(settings.Root);
			RefreshState();
		}

		void OpenFolder()
		{
			Process.Start("explorer.exe", Util.Quote(CurrentFolders.Root));
		}

		void OpenLog()
		{
			string log = CurrentFolders.LauncherLog;
			if (File.Exists(log))
				Process.Start("notepad.exe", Util.Quote(log));
			else
				MessageBox.Show(this, "There is no log yet.", Title, MessageBoxButtons.OK, MessageBoxIcon.Information);
		}

		void ToggleDeveloper()
		{
			bool developer = !settings.DeveloperBuild;
			string question = developer
				? "A developer build stops at the first failed assertion, like Bungie's own debug build: good for testing the port, not for playing.\r\n\r\nClick OK to rebuild Halo as a developer build now."
				: "Click OK to rebuild Halo as a normal build now.";
			if (MessageBox.Show(this, question, Title, MessageBoxButtons.OKCancel, MessageBoxIcon.Question) != DialogResult.OK)
				return;
			settings.DeveloperBuild = developer;
			settings.Save();
			RunWork(false);
		}

		void Uninstall()
		{
			if (Uninstaller.Run(this, settings))
				Close();
		}

		void OnClosing(object sender, FormClosingEventArgs e)
		{
			if (!progress.Busy)
				return;
			if (MessageBox.Show(this, "The launcher is still working. Stop it and close?", Title, MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
			{
				e.Cancel = true;
				return;
			}
			progress.Cancel();
			progress.WaitForExit(8000);
		}
	}

	static class SettingsExtensions
	{
		public static Folders Folders(this Settings settings)
		{
			return new Folders(settings.Root);
		}
	}

	sealed class GameSettingsForm : Form
	{
		static readonly string[][] Languages =
		{
			new[] { "", "Automatic (English)" },
			new[] { "en", "English" },
			new[] { "de", "Deutsch" },
			new[] { "fr", "Fran\u00e7ais" },
			new[] { "es", "Espa\u00f1ol" },
			new[] { "it", "Italiano" },
			new[] { "ja", "Japanese" },
		};

		readonly Settings settings;
		readonly ComboBox scaleBox = new ComboBox();
		readonly ComboBox languageBox = new ComboBox();
		readonly NumericUpDown sensitivityBox = new NumericUpDown();
		readonly NumericUpDown volumeBox = new NumericUpDown();
		readonly CheckBox invertBox = new CheckBox();
		readonly CheckBox classicBox = new CheckBox();
		readonly CheckBox vsyncBox = new CheckBox();
		readonly TextBox extraBox = new TextBox();

		public GameSettingsForm(Settings settings)
		{
			this.settings = settings;
			SuspendLayout();
			AutoScaleDimensions = new SizeF(96F, 96F);
			AutoScaleMode = AutoScaleMode.Dpi;
			Font = new Font("Segoe UI", 9F);
			Text = "Game settings";
			FormBorderStyle = FormBorderStyle.FixedDialog;
			MaximizeBox = false;
			MinimizeBox = false;
			StartPosition = FormStartPosition.CenterParent;
			AutoSize = true;
			AutoSizeMode = AutoSizeMode.GrowAndShrink;

			// sizes are set here, after the scaling mode: the window scales them
			scaleBox.DropDownStyle = ComboBoxStyle.DropDownList;
			scaleBox.Width = 220;
			languageBox.DropDownStyle = ComboBoxStyle.DropDownList;
			languageBox.Width = 220;
			sensitivityBox.Minimum = 0.1m;
			sensitivityBox.Maximum = 10m;
			sensitivityBox.Increment = 0.1m;
			sensitivityBox.DecimalPlaces = 1;
			sensitivityBox.Width = 80;
			volumeBox.Minimum = 0;
			volumeBox.Maximum = 100;
			volumeBox.Increment = 5;
			volumeBox.Width = 80;
			invertBox.Text = "Invert vertical mouse aim";
			classicBox.Text = "Original 30 frames per second (no frames drawn between ticks)";
			vsyncBox.Text = "Do not wait for the display between frames (vsync off)";
			foreach (CheckBox box in new[] { invertBox, classicBox, vsyncBox })
				box.AutoSize = true;
			extraBox.Multiline = true;
			extraBox.ScrollBars = ScrollBars.Vertical;
			extraBox.Size = new Size(380, 80);

			var grid = new TableLayoutPanel { AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, ColumnCount = 2, Padding = new Padding(12) };
			grid.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			grid.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			for (int scale = 1; scale <= 4; scale++)
				scaleBox.Items.Add(string.Format("{0} x {1}  ({2}x)", 640 * scale, 480 * scale, scale));
			foreach (string[] language in Languages)
				languageBox.Items.Add(language[1]);
			Add(grid, "Window size", scaleBox);
			Add(grid, "Mouse sensitivity", sensitivityBox);
			Add(grid, "", invertBox);
			Add(grid, "Volume (%)", volumeBox);
			Add(grid, "Language", languageBox);
			Add(grid, "", classicBox);
			Add(grid, "", vsyncBox);
			Add(grid, "More settings", new Label { Text = "One NAME=value per line, for the HALO_* variables in port/linux/README.md (system link addresses, for example):", AutoSize = true, MaximumSize = new Size(380, 0) });
			Add(grid, "", extraBox);

			var buttons = new FlowLayoutPanel { FlowDirection = FlowDirection.RightToLeft, Dock = DockStyle.Fill, AutoSize = true, WrapContents = false };
			var cancel = new Button { Text = "Cancel", DialogResult = DialogResult.Cancel, AutoSize = true };
			var ok = new Button { Text = "OK", AutoSize = true };
			ok.Click += delegate { if (Store()) { DialogResult = DialogResult.OK; } };
			buttons.Controls.Add(cancel);
			buttons.Controls.Add(ok);
			AcceptButton = ok;
			CancelButton = cancel;
			grid.RowCount++;
			grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			grid.Controls.Add(buttons, 0, grid.RowCount - 1);
			grid.SetColumnSpan(buttons, 2);
			Controls.Add(grid);

			scaleBox.SelectedIndex = settings.WindowScale - 1;
			sensitivityBox.Value = Math.Max(sensitivityBox.Minimum, Math.Min(sensitivityBox.Maximum, settings.MouseSensitivity));
			invertBox.Checked = settings.InvertMouse;
			volumeBox.Value = settings.VolumePercent;
			int languageIndex = Array.FindIndex(Languages, l => l[0] == settings.Language);
			languageBox.SelectedIndex = languageIndex < 0 ? 0 : languageIndex;
			classicBox.Checked = settings.Classic30Fps;
			vsyncBox.Checked = settings.NoVsync;
			extraBox.Text = string.Join("\r\n", settings.Extra.Select(p => p.Key + "=" + p.Value));
			ResumeLayout(true);
		}

		static void Add(TableLayoutPanel grid, string caption, Control control)
		{
			grid.RowCount++;
			grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			grid.Controls.Add(new Label { Text = caption, AutoSize = true, Margin = new Padding(3, 6, 12, 3) }, 0, grid.RowCount - 1);
			control.Margin = new Padding(3, 3, 3, 3);
			grid.Controls.Add(control, 1, grid.RowCount - 1);
		}

		bool Store()
		{
			var extra = new List<KeyValuePair<string, string>>();
			foreach (string raw in extraBox.Lines)
			{
				string line = raw.Trim();
				if (line.Length == 0)
					continue;
				int equals = line.IndexOf('=');
				string name = equals > 0 ? line.Substring(0, equals).Trim() : "";
				if (!Regex.IsMatch(name, "^[A-Za-z_][A-Za-z0-9_]*$"))
				{
					MessageBox.Show(this, "\"" + line + "\" is not NAME=value.", Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
					return false;
				}
				extra.Add(new KeyValuePair<string, string>(name, line.Substring(equals + 1).Trim()));
			}
			settings.WindowScale = scaleBox.SelectedIndex + 1;
			settings.MouseSensitivity = sensitivityBox.Value;
			settings.InvertMouse = invertBox.Checked;
			settings.VolumePercent = (int)volumeBox.Value;
			settings.Language = Languages[Math.Max(0, languageBox.SelectedIndex)][0];
			settings.Classic30Fps = classicBox.Checked;
			settings.NoVsync = vsyncBox.Checked;
			settings.Extra = extra;
			return true;
		}
	}

	/* ---------- without the window: HaloLauncher.exe --install and friends */

	sealed class ConsoleReport : IReport, IDisposable
	{
		readonly StreamWriter file;
		readonly bool yes;
		int lastTenth = -1;

		// logPath null: the console only
		public ConsoleReport(string logPath, bool yes)
		{
			this.yes = yes;
			if (logPath == null)
				return;
			try
			{
				Directory.CreateDirectory(Path.GetDirectoryName(logPath));
				file = new StreamWriter(logPath, true, new UTF8Encoding(false)) { AutoFlush = true };
			}
			catch (Exception)
			{
			}
		}

		public void Log(string line)
		{
			lock (this)
			{
				Console.WriteLine(line);
				if (file != null)
					file.WriteLine(line);
			}
		}

		public void Status(string text)
		{
			lastTenth = -1;
			Log("== " + text);
		}

		public void Step(InstallStep step)
		{
		}

		public void Progress(long done, long total)
		{
			if (total <= 0)
				return;
			int tenth = (int)(done * 10 / total);
			if (tenth != lastTenth)
			{
				lastTenth = tenth;
				Log("   " + tenth * 10 + "%");
			}
		}

		public bool IsCancelled { get { return false; } }

		public bool Ask(string question)
		{
			Log("? " + question.Replace("\r\n", " "));
			Log(yes ? "  (--yes given: yes)" : "  (no: pass --yes to agree)");
			return yes;
		}

		public void Dispose()
		{
			if (file != null)
				file.Dispose();
		}
	}

	static class CommandLine
	{
		[DllImport("kernel32.dll")]
		static extern bool AttachConsole(int processId);

		public const string Usage =
			"HaloLauncher.exe [--play]\r\n" +
			"HaloLauncher.exe --install [--update] [--root DIR] [--data PATH] [--developer] [--clang download|DIR] [--yes] [--no-shortcuts]\r\n" +
			"HaloLauncher.exe --check-data PATH\r\n" +
			"HaloLauncher.exe --write-icon FILE.ico";

		public static int Run(Settings settings, string[] args)
		{
			AttachConsole(-1);
			string command = args[0];
			string root = null, data = null, clang = null, value = null;
			bool developer = false, yes = false, update = false, shortcuts = true;
			for (int i = 1; i < args.Length; i++)
			{
				bool hasNext = i + 1 < args.Length;
				switch (args[i])
				{
				case "--root": root = hasNext ? args[++i] : null; break;
				case "--xdk":
					// (earlier launchers took the Xbox SDK; scripts may still pass it)
					if (hasNext)
						i++;
					Console.WriteLine("--xdk is ignored: the Xbox development kit isn't needed any more.");
					break;
				case "--data": data = hasNext ? args[++i] : null; break;
				case "--clang": clang = hasNext ? args[++i] : null; break;
				case "--developer": developer = true; break;
				case "--yes": yes = true; break;
				case "--update": update = true; break;
				case "--no-shortcuts": shortcuts = false; break;
				default:
					if (value != null || args[i].StartsWith("--"))
					{
						Console.WriteLine(Usage);
						return 2;
					}
					value = args[i];
					break;
				}
			}
			// nothing is written into an install folder before it is checked
			bool install = command == "--install";
			if (root != null || install)
			{
				string full;
				string problem = install ? Folders.Problem(root ?? settings.Root, out full) : Folders.PathProblem(root, out full);
				if (problem != null)
				{
					Console.WriteLine(problem);
					return 2;
				}
				if (root != null)
					settings = Settings.ForRoot(full);
			}
			if (data != null)
				settings.DataInput = data;
			if (developer)
				settings.DeveloperBuild = true;
			if (clang != null)
				settings.Clang = clang;
			using (var report = new ConsoleReport(install ? settings.Folders().LauncherLog : null, yes))
			{
				try
				{
					switch (command)
					{
					case "--install":
						Installer.Run(settings, report, update || !File.Exists(Path.Combine(settings.Folders().Game, "configure.py")), shortcuts);
						return 0;
					case "--write-icon":
						// (build-exe.cmd builds the launcher with its icon)
						AppIcon.Save(value ?? "HaloLauncher.ico");
						return 0;
					case "--check-xdk":
						report.Log("The Xbox development kit isn't needed any more: Halo builds with the clean SDK declarations in its source (port/include/xdk).");
						return 0;
					case "--check-data":
						using (GameDataSource source = GameData.Open(value ?? "", Path.GetTempPath(), report, false))
						{
							if (source == null)
							{
								report.Log("No Halo game data found in " + value + ".");
								return 1;
							}
							if (source is ArchiveSource)
							{
								report.Log(source.Description);
								return 0;
							}
							foreach (var group in source.Maps.GroupBy(m => m.Folder + " " + m.Build))
								report.Log(string.Format("  {0}: {1} maps", group.Key, group.Count()));
							string problem = GameData.Problem(source.Maps);
							report.Log(problem ?? source.Summary());
							if (problem == null && source.Build == Pinned.NtscBuild)
								report.Log("Installing it adds this build to the game's two map checks (source\\cache).");
							return problem == null ? 0 : 1;
						}
					default:
						Console.WriteLine(Usage);
						return 2;
					}
				}
				catch (UserError error)
				{
					report.Log("!! " + error.Message);
					return 1;
				}
				catch (Exception error)
				{
					report.Log("!! " + error);
					return 1;
				}
			}
		}
	}

	static class Program
	{
		[DllImport("user32.dll")]
		static extern bool SetProcessDPIAware();

		[STAThread]
		static int Main(string[] args)
		{
			Settings settings = Settings.Load();
			string command = args.Length > 0 ? args[0] : "";
			if (command.Length > 0 && command != "--play" && command != "--uninstall")
				return CommandLine.Run(settings, args);

			try
			{
				SetProcessDPIAware();
			}
			catch (Exception)
			{
			}
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			// an unexpected error in the window is reported, and the window stays
			Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
			Application.ThreadException += delegate(object sender, ThreadExceptionEventArgs e)
			{
				MessageBox.Show("Something went wrong: " + e.Exception.Message, "Halo CE Universal", MessageBoxButtons.OK, MessageBoxIcon.Warning);
			};
			var tidy = new Thread(Util.RemoveOldChecks);
			tidy.IsBackground = true;
			tidy.Start();
			// Windows' installed apps start this to uninstall
			if (command == "--uninstall")
			{
				Uninstaller.Run(null, settings);
				return 0;
			}
			Folders folders = settings.Folders();
			// --play starts the game at once; a window opens instead when
			// something is missing
			if (command == "--play" && folders.GameBuilt && GameData.IsInstalled(folders.Data))
			{
				GameLauncher.Start(folders, settings).Dispose();
				return 0;
			}
			// the first time, setup; after it, the launcher
			if (!folders.GameBuilt)
			{
				var setup = new SetupWizard(settings, false);
				Application.Run(setup);
				if (!setup.OpenLauncher)
					return 0;
				settings = Settings.ForRoot(setup.Root);
			}
			Application.Run(new MainForm(settings));
			return 0;
		}
	}
}
