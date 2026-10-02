#!/bin/sh
# Compiles the Metal ray tracing kernels (host_metal_rt.m's kernel_source)
# through the Metal runtime, as the game does at start, without the Metal
# toolchain and without starting the game: "kernel compiles: N functions",
# or the compiler's errors.
set -e
cd "$(dirname "$0")/../../.."
out=build/macos/kernel_check
mkdir -p "$out"
python3 - "$out" <<'PY'
import sys
out = sys.argv[1]
lines = open('port/macos/host/host_metal_rt.m').read().split('\n')
start = next(i for i, l in enumerate(lines) if 'static NSString *const kernel_source' in l)
end = start
while not lines[end].rstrip().endswith('";'):
	end += 1
main = r'''
int main(void)
{
	@autoreleasepool
	{
		id<MTLDevice> device = MTLCreateSystemDefaultDevice();
		NSError *error = nil;
		id<MTLLibrary> library = [device newLibraryWithSource:kernel_source options:nil error:&error];

		if (!library)
		{
			printf("the kernel does not build: %s\n", error.localizedDescription.UTF8String);
			return 1;
		}
		printf("kernel compiles: %lu functions\n", (unsigned long)library.functionNames.count);
		return 0;
	}
}
'''
open(out + '/kernel_check.m', 'w').write('#import <Foundation/Foundation.h>\n#import <Metal/Metal.h>\n' +
	'\n'.join(lines[start:end + 1]) + '\n' + main)
PY
clang -fobjc-arc -framework Foundation -framework Metal "$out/kernel_check.m" -o "$out/kernel_check"
"$out/kernel_check"
