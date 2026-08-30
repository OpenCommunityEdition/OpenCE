/* CRT-free 32-bit debugger for c2.dll, built with VC7.
   Same-bitness (32-bit debugging 32-bit) so INT3 + native x86 CONTEXT work,
   unlike the 64-bit host. Reads child cmdline/workdir from UTF-16LE files and
   breakpoint VAs from an ASCII file; writes register/memory dumps on each hit
   to dbg_result.txt. */

typedef unsigned char BYTE; typedef unsigned short WORD; typedef unsigned long DWORD;
typedef int BOOL; typedef void* HANDLE; typedef void* LPVOID; typedef unsigned short WCHAR;

#define NULL ((void*)0)
#define INFINITE 0xFFFFFFFF
#define DBG_CONTINUE 0x00010002
#define DBG_EXCEPTION_NOT_HANDLED 0x80010001
#define DEBUG_ONLY_THIS_PROCESS 0x00000002
#define CREATE_NEW_CONSOLE 0x00000010
#define CTX_FULL 0x00010007
#define CTX_DBG  0x00010010          /* CONTEXT_DEBUG_REGISTERS */
#define CTX_ALL  0x00010017          /* FULL | DEBUG_REGISTERS  */
#define TF 0x100
#define EXC_BP 0x80000003
#define EXC_SS 0x80000004
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define OPEN_EXISTING 3
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_NORMAL 0x80
#define PAGE_EXECUTE_READWRITE 0x40

__declspec(dllimport) HANDLE __stdcall CreateFileW(const WCHAR*,DWORD,DWORD,LPVOID,DWORD,DWORD,HANDLE);
__declspec(dllimport) DWORD  __stdcall GetFileSize(HANDLE,DWORD*);
__declspec(dllimport) BOOL   __stdcall ReadFile(HANDLE,LPVOID,DWORD,DWORD*,LPVOID);
__declspec(dllimport) BOOL   __stdcall WriteFile(HANDLE,const void*,DWORD,DWORD*,LPVOID);
__declspec(dllimport) BOOL   __stdcall CloseHandle(HANDLE);
__declspec(dllimport) BOOL   __stdcall CreateProcessW(const WCHAR*,WCHAR*,LPVOID,LPVOID,BOOL,DWORD,LPVOID,const WCHAR*,LPVOID,LPVOID);
__declspec(dllimport) BOOL   __stdcall WaitForDebugEvent(LPVOID,DWORD);
__declspec(dllimport) BOOL   __stdcall ContinueDebugEvent(DWORD,DWORD,DWORD);
__declspec(dllimport) BOOL   __stdcall ReadProcessMemory(HANDLE,LPVOID,LPVOID,DWORD,DWORD*);
__declspec(dllimport) BOOL   __stdcall WriteProcessMemory(HANDLE,LPVOID,const void*,DWORD,DWORD*);
__declspec(dllimport) BOOL   __stdcall GetThreadContext(HANDLE,LPVOID);
__declspec(dllimport) BOOL   __stdcall SetThreadContext(HANDLE,LPVOID);
__declspec(dllimport) HANDLE __stdcall OpenThread(DWORD,BOOL,DWORD);
__declspec(dllimport) BOOL   __stdcall VirtualProtectEx(HANDLE,LPVOID,DWORD,DWORD,DWORD*);
__declspec(dllimport) DWORD  __stdcall VirtualQueryEx(HANDLE,LPVOID,LPVOID,DWORD);
__declspec(dllimport) BOOL   __stdcall FlushInstructionCache(HANDLE,LPVOID,DWORD);
__declspec(dllimport) void   __stdcall ExitProcess(unsigned);
__declspec(dllimport) DWORD  __stdcall GetLastError(void);

/* --- x86 structures --- */
typedef struct { DWORD cb; WCHAR* r0; WCHAR* r1; WCHAR* r2;
  DWORD dwX,dwY,dwXSize,dwYSize,dwXCC,dwYCC,dwFill,dwFlags;
  WORD wShow,cbR2; LPVOID lpR2; HANDLE hIn,hOut,hErr; } STARTUPINFOW;
typedef struct { HANDLE hProcess,hThread; DWORD pid,tid; } PROCESS_INFORMATION;

typedef struct { DWORD Code,Flags; LPVOID Rec; LPVOID Addr; DWORD nParams; DWORD Info[15]; } EXC_RECORD;
typedef struct { EXC_RECORD ExceptionRecord; DWORD dwFirstChance; } EXC_INFO;
typedef struct { HANDLE hFile,hProcess,hThread; LPVOID base; DWORD off,sz; LPVOID tlb,start,name; WORD uni; } CP_INFO;
typedef struct { HANDLE hFile; LPVOID base; DWORD off,sz; LPVOID name; WORD uni; } LD_INFO;
typedef struct { DWORD dwExitCode; } EP_INFO;
typedef struct {
  DWORD code, pid, tid;
  union { EXC_INFO Exception; CP_INFO CreateProcessInfo; LD_INFO LoadDll; EP_INFO ExitProcess; BYTE raw[160]; } u;
} DEBUG_EVENT;

typedef struct { DWORD cw,sw,tw,eo,es,do_,ds; BYTE reg[80]; DWORD cr0; } FSAVE;
typedef struct {
  DWORD ContextFlags; DWORD Dr0,Dr1,Dr2,Dr3,Dr6,Dr7; FSAVE FloatSave;
  DWORD SegGs,SegFs,SegEs,SegDs; DWORD Edi,Esi,Ebx,Edx,Ecx,Eax;
  DWORD Ebp,Eip,SegCs,EFlags,Esp,SegSs; BYTE Ext[512];
} CONTEXT;

/* --- tiny CRT --- */
#pragma function(memset)
void* __cdecl memset(void* d,int c,unsigned n){BYTE* p=(BYTE*)d;while(n--)*p++=(BYTE)c;return d;}

static HANDLE g_out;
static char ob[4096]; static int oi;
static void flush(void){ DWORD w; if(oi){WriteFile(g_out,ob,(DWORD)oi,&w,NULL);oi=0;} }
static void putc_(char c){ if(oi>=(int)sizeof(ob))flush(); ob[oi++]=c; }
static void puts_(const char* s){ while(*s)putc_(*s++); }
static void hex_(DWORD v,int digits){ char t[8]; int i; for(i=0;i<8;i++){int nib=(v>>((7-i)*4))&0xf; t[i]=(char)(nib<10?'0'+nib:'a'+nib-10);} for(i=8-digits;i<8;i++)putc_(t[i]); }
static void hx(DWORD v){ puts_("0x"); hex_(v,8); }

/* --- config buffers --- */
static WCHAR cmdw[16384];
static WCHAR dirw[4096];
static char bpbuf[4096];
static DWORD bps[8]; static BYTE orig[8]; static int nbp;
static DWORD hits[8];
static int maxhits=250;
/* gate mode: bp[0] is a per-function counter (uncapped, lightweight); the
   decision bps (bp[1..]) only heavy-dump while the gate count is in
   [g_gate_lo, g_gate_hi]. Read from dbg_gate.txt as "lo hi" (decimal). If the
   file is absent (g_gate_lo==0), gate mode is off and all bps behave normally. */
static DWORD g_gate_lo=0, g_gate_hi=0, g_gate_count=0;
static char gatebuf[64];

/* --- hardware data watchpoints (x86 debug registers DR0..DR3) ---
   dbg_wp.txt holds up to four "<hexaddr> <declen>" pairs; len is 1, 2 or 4
   bytes. Each becomes a WRITE watchpoint, so the trap fires immediately after
   an instruction stores to that address, with the new value already in memory
   and Eip pointing at the following instruction. This is the instrument for
   "who writes this field?", which INT3 breakpoints on guessed code addresses
   cannot answer. */
static char wpbuf[256];
static DWORD wps[4]; static DWORD wplen[4]; static int nwp;
static DWORD wphits[4];

/* --- memory pattern scan ---
   dbg_scan.txt holds a hex byte string ("558bec5657") plus an optional decimal
   "after N": on the Nth breakpoint hit (default: every hit) sweep the
   debuggee's committed private/image pages for that byte sequence and print the
   addresses. This answers "where in memory is the code being assembled?", which
   neither INT3 nor a watchpoint can, because the buffer address is what you are
   looking for in the first place. */
/* --- arm a watchpoint at a RUNTIME-DISCOVERED address (dbg_armwp.txt) ---
   "<hexpattern> @N +<hexoff> <len>": at the Nth hit of bp0, scan the
   debuggee for <hexpattern>; on the first match, program a write
   watchpoint at match+<hexoff>.  This is the instrument for "who writes
   this byte?" when the address only exists at run time -- arena/heap
   buffers move between runs, so such an address cannot be written into
   dbg_wp.txt up front. */
/* dbg_armreg.txt: "<reg> <hexoff> <hit> <len>" e.g. "ebp 18 3 4" --
   at the Nth hit of bp0, take <reg> from the trapping context and arm a
   write watchpoint at reg+off.  IR node addresses shift ~1MB per run, so
   this is the only way to watch a node field. */
static char aregbuf[128];
static int areg_idx=-1; static DWORD areg_off=0, areg_hit=0, areg_len=4;

static char armbuf[256];
static BYTE armpat[64]; static int armlen=0;
static DWORD arm_after=0, arm_off=0, arm_wplen=1; static int armed=0;

static char scanbuf[512];
static BYTE scanpat[64]; static int scanlen=0; static DWORD scan_after=0;
static BYTE scanwin[4096];
static HANDLE g_proc;
typedef struct { LPVOID Base; LPVOID Alloc; DWORD AllocProt, RegionSize, State, Protect, Type; } MBI;

static void do_scan(void){
  MBI m; DWORD addr=0x10000; int found=0;
  if(scanlen<=0) return;
  puts_("SCAN for "); hex_((DWORD)scanlen,1); puts_(" bytes:\n");
  while(addr < 0x7ff00000){
    if(VirtualQueryEx(g_proc,(LPVOID)addr,&m,sizeof(m))!=sizeof(m)) break;
    if(m.State==0x1000 /*MEM_COMMIT*/ && !(m.Protect & 0x101) /*not NOACCESS/GUARD*/){
      DWORD base=(DWORD)m.Base, size=m.RegionSize, off=0;
      while(off < size){
        DWORD want = (size-off > sizeof(scanwin)) ? (DWORD)sizeof(scanwin) : (size-off);
        DWORD rd=0;
        if(ReadProcessMemory(g_proc,(LPVOID)(base+off),scanwin,want,&rd) && rd>=(DWORD)scanlen){
          DWORD i; int j;
          for(i=0;i+(DWORD)scanlen<=rd;i++){
            for(j=0;j<scanlen;j++) if(scanwin[i+j]!=scanpat[j]) break;
            if(j==scanlen){ puts_("   hit @"); hx(base+off+i); putc_('\n'); found++; if(found>=32){ flush(); return; } }
          }
        }
        if(want<=(DWORD)scanlen) break;
        off += want-(DWORD)scanlen;   /* overlap so matches spanning the seam are seen */
      }
    }
    addr = (DWORD)m.Base + m.RegionSize;
    if(m.RegionSize==0) break;
  }
  if(!found) puts_("   (no match)\n");
  flush();
}

/* First address in the debuggee matching pat[0..patlen), or 0. Same sweep
   as do_scan but returns instead of printing. */
static DWORD find_first(const BYTE* pat, int patlen){
  MBI m; DWORD addr=0x10000;
  if(patlen<=0) return 0;
  while(addr < 0x7ff00000){
    if(VirtualQueryEx(g_proc,(LPVOID)addr,&m,sizeof(m))!=sizeof(m)) break;
    if(m.State==0x1000 && !(m.Protect & 0x101)){
      DWORD base=(DWORD)m.Base, size=m.RegionSize, off=0;
      while(off < size){
        DWORD want = (size-off > sizeof(scanwin)) ? (DWORD)sizeof(scanwin) : (size-off);
        DWORD rd=0;
        if(ReadProcessMemory(g_proc,(LPVOID)(base+off),scanwin,want,&rd) && rd>=(DWORD)patlen){
          DWORD i; int j;
          for(i=0;i+(DWORD)patlen<=rd;i++){
            for(j=0;j<patlen;j++) if(scanwin[i+j]!=pat[j]) break;
            if(j==patlen) return base+off+i;
          }
        }
        if(want<=(DWORD)patlen) break;
        off += want-(DWORD)patlen;
      }
    }
    addr = (DWORD)m.Base + m.RegionSize;
    if(m.RegionSize==0) break;
  }
  return 0;
}

static int readfile(const WCHAR* path, void* buf, int cap){
  HANDLE h=CreateFileW(path,GENERIC_READ,1,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  DWORD sz,rd; if(h==(HANDLE)-1)return -1;
  sz=GetFileSize(h,NULL); if((int)sz>cap)sz=cap;
  ReadFile(h,buf,sz,&rd,NULL); CloseHandle(h); return (int)rd;
}

static DWORD parsehex(const char* s){ DWORD v=0; while(*s==' '||*s=='\t'||*s=='\n'||*s=='\r')s++;
  if(s[0]=='0'&&(s[1]=='x'||s[1]=='X'))s+=2;
  for(;;){char c=*s++; int d; if(c>='0'&&c<='9')d=c-'0'; else if(c>='a'&&c<='f')d=c-'a'+10;
    else if(c>='A'&&c<='F')d=c-'A'+10; else break; v=v*16+d;} return v; }

/* parse a decimal; advance *pp past it and any following separators */
static DWORD parsedec(char** pp){ char* s=*pp; DWORD v=0; while(*s==' '||*s=='\t'||*s=='\n'||*s=='\r')s++;
  for(;;){char c=*s; if(c<'0'||c>'9')break; v=v*10+(DWORD)(c-'0'); s++;}
  while(*s==' '||*s=='\t')s++; *pp=s; return v; }

/* config file paths (UTF-16LE literals) */
static const WCHAR P_CMD[] = {'d','b','g','_','c','m','d','.','u','1','6',0};
static const WCHAR P_DIR[] = {'d','b','g','_','d','i','r','.','u','1','6',0};
static const WCHAR P_BP[]  = {'d','b','g','_','b','p','.','t','x','t',0};
static const WCHAR P_OUT[] = {'d','b','g','_','r','e','s','u','l','t','.','t','x','t',0};
static const WCHAR P_GATE[]= {'d','b','g','_','g','a','t','e','.','t','x','t',0};
static const WCHAR P_WP[]  = {'d','b','g','_','w','p','.','t','x','t',0};
static const WCHAR P_AREG[]= {'d','b','g','_','a','r','m','r','e','g','.','t','x','t',0};
static const WCHAR P_ARM[] = {'d','b','g','_','a','r','m','w','p','.','t','x','t',0};
static const WCHAR P_SCAN[]= {'d','b','g','_','s','c','a','n','.','t','x','t',0};

static DWORD pend_tid[16]; static DWORD pend_addr[16]; static int npend;



static BYTE membuf[160];

/* Program DR0..DR3 for the watch list on one thread. RW=01 (write-only) and
   LEN encoded 00/01/11 for 1/2/4 bytes; a 4-byte watch must be 4-byte aligned
   and a 2-byte watch 2-byte aligned, which the caller is responsible for. */
static void arm_wp(HANDLE hth){
  CONTEXT c; DWORD dr7=0; int i;
  if(nwp<=0) return;
  memset(&c,0,sizeof(c)); c.ContextFlags=CTX_DBG;
  if(!GetThreadContext(hth,&c)) return;
  for(i=0;i<nwp;i++){
    DWORD lenbits = (wplen[i]==4)?3u : (wplen[i]==2)?1u : 0u;
    if(i==0)c.Dr0=wps[i]; else if(i==1)c.Dr1=wps[i];
    else if(i==2)c.Dr2=wps[i]; else c.Dr3=wps[i];
    dr7 |= (1u<<(2*i));                     /* Ln: local enable          */
    dr7 |= (1u<<(16+4*i));                  /* R/Wn = 01 -> data write   */
    dr7 |= (lenbits<<(18+4*i));             /* LENn                      */
  }
  c.Dr6=0; c.Dr7=dr7;
  c.ContextFlags=CTX_DBG; SetThreadContext(hth,&c);
}

static void plant(HANDLE proc, DWORD va, BYTE b){
  DWORD old,w; VirtualProtectEx(proc,(LPVOID)va,1,PAGE_EXECUTE_READWRITE,&old);
  WriteProcessMemory(proc,(LPVOID)va,&b,1,&w);
  VirtualProtectEx(proc,(LPVOID)va,1,old,&old);
  FlushInstructionCache(proc,(LPVOID)va,1);
}

void entry(void){
  STARTUPINFOW si; PROCESS_INFORMATION pi; DEBUG_EVENT de; CONTEXT ctx;
  int n,i; DWORD w; DWORD status;
  HANDLE hth;

  memset(&si,0,sizeof(si)); memset(&pi,0,sizeof(pi));
  g_out=CreateFileW(P_OUT,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

  readfile(P_CMD,cmdw,sizeof(cmdw)-2);
  readfile(P_DIR,dirw,sizeof(dirw)-2);
  n=readfile(P_BP,bpbuf,sizeof(bpbuf)-1); if(n<0)n=0; bpbuf[n]=0;
  { char* p=bpbuf; while(*p&&nbp<8){ while(*p==' '||*p=='\t'||*p=='\n'||*p=='\r')p++;
      if(!*p)break; bps[nbp++]=parsehex(p); while(*p&&*p!=' '&&*p!='\t'&&*p!='\n'&&*p!='\r')p++; } }

  n=readfile(P_GATE,gatebuf,sizeof(gatebuf)-1); if(n<0)n=0; gatebuf[n]=0;
  { char* p=gatebuf; g_gate_lo=parsedec(&p); g_gate_hi=parsedec(&p); if(g_gate_hi<g_gate_lo)g_gate_hi=g_gate_lo; }

  n=readfile(P_WP,wpbuf,sizeof(wpbuf)-1); if(n<0)n=0; wpbuf[n]=0;
  { char* p=wpbuf; while(*p&&nwp<4){
      DWORD a,l;
      while(*p==' '||*p=='\t'||*p=='\n'||*p=='\r')p++;
      if(!*p)break;
      a=parsehex(p); while(*p&&*p!=' '&&*p!='\t'&&*p!='\n'&&*p!='\r')p++;
      l=parsedec(&p); if(l!=1&&l!=2&&l!=4)l=1;
      wps[nwp]=a; wplen[nwp]=l; nwp++; } }

  n=readfile(P_SCAN,scanbuf,sizeof(scanbuf)-1); if(n<0)n=0; scanbuf[n]=0;
  { char* p=scanbuf; int hi=-1;
    while(*p && scanlen<(int)sizeof(scanpat)){
      char c=*p; int d=-1;
      if(c>='0'&&c<='9')d=c-'0'; else if(c>='a'&&c<='f')d=c-'a'+10;
      else if(c>='A'&&c<='F')d=c-'A'+10;
      else { if(c=='@'){ p++; scan_after=parsedec(&p); break; } p++; continue; }
      if(hi<0) hi=d; else { scanpat[scanlen++]=(BYTE)((hi<<4)|d); hi=-1; }
      p++; } }

  n=readfile(P_ARM,armbuf,sizeof(armbuf)-1); if(n<0)n=0; armbuf[n]=0;
  { char* p=armbuf; int hi=-1;
    while(*p && armlen<(int)sizeof(armpat)){
      char c=*p; int d=-1;
      if(c>='0'&&c<='9')d=c-'0'; else if(c>='a'&&c<='f')d=c-'a'+10;
      else if(c>='A'&&c<='F')d=c-'A'+10;
      else break;
      if(hi<0) hi=d; else { armpat[armlen++]=(BYTE)((hi<<4)|d); hi=-1; }
      p++; }
    while(*p){
      if(*p=='@'){ p++; arm_after=parsedec(&p); continue; }
      if(*p=='+'){ p++; arm_off=parsehex(p);
        while(*p&&*p!=' '&&*p!='\t'&&*p!='\n'&&*p!='\r')p++; continue; }
      if(*p>='1'&&*p<='9'){ arm_wplen=parsedec(&p); continue; }
      p++; }
    if(arm_wplen!=1&&arm_wplen!=2&&arm_wplen!=4) arm_wplen=1;
    if(arm_after==0) arm_after=1; }

  n=readfile(P_AREG,aregbuf,sizeof(aregbuf)-1); if(n<0)n=0; aregbuf[n]=0;
  if(aregbuf[0]){ char* p=aregbuf; char r0,r1,r2;
    while(*p==' ')p++;
    r0=p[0]; r1=p[1]; r2=p[2];
    if(r0=='e'){
      if(r1=='a'&&r2=='x')areg_idx=0; else if(r1=='b'&&r2=='x')areg_idx=1;
      else if(r1=='c'&&r2=='x')areg_idx=2; else if(r1=='d'&&r2=='x')areg_idx=3;
      else if(r1=='s'&&r2=='i')areg_idx=4; else if(r1=='d'&&r2=='i')areg_idx=5;
      else if(r1=='b'&&r2=='p')areg_idx=6; else if(r1=='s'&&r2=='p')areg_idx=7; }
    while(*p&&*p!=' ')p++;
    areg_off=parsehex(p); while(*p==' ')p++;
    while(*p&&*p!=' ')p++;
    areg_hit=parsedec(&p); areg_len=parsedec(&p);
    if(areg_len!=1&&areg_len!=2&&areg_len!=4)areg_len=4;
    if(areg_hit==0)areg_hit=1; }

  puts_("bp count="); hex_((DWORD)nbp,1); putc_('\n');
  for(i=0;i<nbp;i++){ puts_("  bp"); hex_((DWORD)i,1); puts_("="); hx(bps[i]); putc_('\n'); }
  if(armlen>0){ puts_("armwp: scan "); hex_((DWORD)armlen,1);
    puts_(" bytes at bp0 hit "); hex_(arm_after,1); puts_(", watch match+");
    hx(arm_off); putc_('/'); hex_(arm_wplen,1); putc_('\n'); }
  if(areg_idx>=0){ puts_("armreg: at bp0 hit "); hex_(areg_hit,1);
    puts_(" watch reg["); hex_((DWORD)areg_idx,1); puts_("]+"); hx(areg_off);
    putc_('/'); hex_(areg_len,1); putc_('\n'); }
  if(g_gate_lo){ puts_("gate mode: bp0 counts; dump bp1+ while count in ["); hex_(g_gate_lo,1);
    puts_(".."); hex_(g_gate_hi,1); puts_("]\n"); }
  flush();

  si.cb=sizeof(si);
  if(!CreateProcessW(NULL,cmdw,NULL,NULL,0,DEBUG_ONLY_THIS_PROCESS|CREATE_NEW_CONSOLE,NULL,dirw,&si,&pi)){
    puts_("CreateProcessW failed err="); hx(GetLastError()); putc_('\n'); flush(); ExitProcess(1);
  }
  puts_("launched pid="); hx(pi.pid); putc_('\n'); flush();

  for(;;){
    status=DBG_CONTINUE;
    if(!WaitForDebugEvent(&de,INFINITE)) break;
    if(de.code==3){ /* CREATE_PROCESS */
      g_proc=de.u.CreateProcessInfo.hProcess;
    } else if(de.code==6){ /* LOAD_DLL */
      if((DWORD)de.u.LoadDll.base==0x10700000){
        int np = g_gate_lo ? 1 : nbp; /* gate mode: plant only bp0 (the counter) up front */
        for(i=0;i<nbp;i++){ DWORD rd; ReadProcessMemory(g_proc,(LPVOID)bps[i],&orig[i],1,&rd); }
        for(i=0;i<np;i++) plant(g_proc,bps[i],0xCC);
        puts_("c2 loaded @0x10700000, planted "); hex_((DWORD)np,1); puts_(" bps\n");
        if(nwp>0){
          hth=OpenThread(0x1FFFFF,0,de.tid);
          arm_wp(hth); CloseHandle(hth);
          puts_("armed "); hex_((DWORD)nwp,1); puts_(" write watchpoints:");
          for(i=0;i<nwp;i++){ putc_(' '); hx(wps[i]); putc_('/'); hex_(wplen[i],1); }
          putc_('\n');
        }
        flush();
      }
    } else if(de.code==1){ /* EXCEPTION */
      DWORD ecode=de.u.Exception.ExceptionRecord.Code;
      DWORD addr=(DWORD)de.u.Exception.ExceptionRecord.Addr;
      int bi=-1; for(i=0;i<nbp;i++) if(addr==bps[i]) bi=i;
      if(ecode==EXC_BP && bi>=0){
        int heavy=1, uncapped=0;
        hth=OpenThread(0x1FFFFF,0,de.tid);
        memset(&ctx,0,sizeof(ctx)); ctx.ContextFlags=CTX_FULL;
        GetThreadContext(hth,&ctx);
        hits[bi]++;
        if(areg_idx>=0 && !armed && bi==0 && hits[0]>=areg_hit){
          DWORD rv[8]; DWORD wa;
          rv[0]=ctx.Eax; rv[1]=ctx.Ebx; rv[2]=ctx.Ecx; rv[3]=ctx.Edx;
          rv[4]=ctx.Esi; rv[5]=ctx.Edi; rv[6]=ctx.Ebp; rv[7]=ctx.Esp;
          wa = rv[areg_idx] + areg_off;
          armed=1;
          puts_("ARMREG at bp0 hit "); hex_(hits[0],1); puts_(": reg=");
          hx(rv[areg_idx]); puts_(" -> watch "); hx(wa); putc_('/');
          hex_(areg_len,1); putc_('\n');
          if(nwp<4){ wps[nwp]=wa; wplen[nwp]=areg_len; nwp++; arm_wp(hth); }
          flush();
        }
        if(armlen>0 && !armed && bi==0 && hits[0]>=arm_after){
          DWORD found=find_first(armpat,armlen);
          armed=1;
          puts_("ARMWP scan at bp0 hit "); hex_(hits[0],1); puts_(": ");
          if(found){
            DWORD wa=found+arm_off;
            hx(found); puts_(" -> watch "); hx(wa); putc_('/');
            hex_(arm_wplen,1); putc_('\n');
            if(nwp<4){ wps[nwp]=wa; wplen[nwp]=arm_wplen; nwp++; arm_wp(hth); }
            else puts_("   (no free debug register)\n");
          } else puts_("no match\n");
          flush();
        }
        if(g_gate_lo && bi==0){ /* the per-function gate: count, arm/disarm window, no heavy dump */
          DWORD cur=0,rd=0; g_gate_count++; heavy=0; uncapped=1;
          if(g_gate_count==g_gate_lo){ for(i=1;i<nbp;i++) plant(g_proc,bps[i],0xCC); }
          if(g_gate_count==g_gate_hi+1){ for(i=1;i<nbp;i++) plant(g_proc,bps[i],orig[i]); }
          ReadProcessMemory(g_proc,(LPVOID)0x1088b788,&cur,4,&rd);
          puts_("GATE #"); hex_(g_gate_count,1); puts_(" eip="); hx(ctx.Eip);
          puts_(" eax="); hx(ctx.Eax); puts_(" ecx="); hx(ctx.Ecx); puts_(" cur="); hx(cur); putc_('\n'); flush();
        }
        if(heavy){
        puts_("HIT bp"); hex_((DWORD)bi,1); puts_(" #"); hex_(hits[bi],1);
        puts_(" eip="); hx(ctx.Eip); puts_(" eax="); hx(ctx.Eax); puts_(" ebx="); hx(ctx.Ebx);
        puts_(" ecx="); hx(ctx.Ecx); puts_(" edx="); hx(ctx.Edx); putc_('\n');
        puts_("        esi="); hx(ctx.Esi); puts_(" edi="); hx(ctx.Edi);
        puts_(" ebp="); hx(ctx.Ebp); puts_(" esp="); hx(ctx.Esp); puts_(" efl="); hx(ctx.EFlags); putc_('\n');
        /* stack dwords */
        { DWORD rd; ReadProcessMemory(g_proc,(LPVOID)ctx.Esp,membuf,32,&rd);
          puts_("        stk:"); for(i=0;i<8;i++){ putc_(' '); hx(((DWORD*)membuf)[i]); } putc_('\n'); }
        /* dump memory each GP register points at (IR node records) */
        { DWORD regs[7]; const char* nm[7]; int r; regs[0]=ctx.Eax;nm[0]="eax";regs[1]=ctx.Ebx;nm[1]="ebx";
          regs[2]=ctx.Ecx;nm[2]="ecx";regs[3]=ctx.Edx;nm[3]="edx";regs[4]=ctx.Esi;nm[4]="esi";regs[5]=ctx.Edi;nm[5]="edi";regs[6]=ctx.Ebp;nm[6]="ebp";
          for(r=0;r<7;r++){ DWORD rd=0; if(regs[r]>0x10000 && regs[r]<0x7ff00000 &&
              ReadProcessMemory(g_proc,(LPVOID)regs[r],membuf,96,&rd) && rd>=96){
              int j; puts_("        ["); puts_(nm[r]); puts_("]@"); hx(regs[r]); puts_(":");
              for(j=0;j<24;j++){ putc_(' '); hx(((DWORD*)membuf)[j]); } putc_('\n'); } } }
        /* recently-emitted output bytes: cursor = [0x1088b788] */
        { DWORD cur=0,rd=0; ReadProcessMemory(g_proc,(LPVOID)0x1088b788,&cur,4,&rd);
          if(cur>0x10000 && ReadProcessMemory(g_proc,(LPVOID)(cur-16),membuf,16,&rd) && rd>=16){
            int j; puts_("        out[-16..cur]@"); hx(cur); puts_(":");
            for(j=0;j<16;j++){ putc_(' '); hex_(membuf[j],2); } putc_('\n'); } }
        flush();
        } /* end if(heavy) */
        if(scanlen>0 && (scan_after==0 || hits[bi]==scan_after)) do_scan();
        /* restore original, back up eip, single-step to re-arm (uncapped for the gate) */
        plant(g_proc,bps[bi],orig[bi]);
        ctx.Eip=bps[bi];
        if(uncapped || hits[bi]<(DWORD)maxhits){ ctx.EFlags|=TF; if(npend<16){pend_tid[npend]=de.tid;pend_addr[npend]=bps[bi];npend++;} }
        ctx.ContextFlags=CTX_FULL; SetThreadContext(hth,&ctx); CloseHandle(hth);
      } else if(ecode==EXC_SS){
        /* A single-step exception is either our INT3 re-arm step (DR6.BS, bit
           14) or a data watchpoint trap (DR6 bits 0..3). Check the watchpoints
           first; the store has already completed, so the watched bytes hold the
           NEW value and Eip is the instruction after the writer. */
        int wi=-1;
        hth=OpenThread(0x1FFFFF,0,de.tid);
        memset(&ctx,0,sizeof(ctx)); ctx.ContextFlags=CTX_ALL;
        GetThreadContext(hth,&ctx);
        for(i=0;i<nwp;i++) if(ctx.Dr6 & (1u<<i)) wi=i;
        if(wi>=0){
          DWORD val=0,rd=0;
          wphits[wi]++;
          ReadProcessMemory(g_proc,(LPVOID)wps[wi],&val,wplen[wi],&rd);
          puts_("WP"); hex_((DWORD)wi,1); puts_(" #"); hex_(wphits[wi],1);
          puts_(" @"); hx(wps[wi]); puts_(" now="); hx(val);
          puts_(" writer_eip="); hx(ctx.Eip); putc_('\n');
          puts_("        eax="); hx(ctx.Eax); puts_(" ebx="); hx(ctx.Ebx);
          puts_(" ecx="); hx(ctx.Ecx); puts_(" edx="); hx(ctx.Edx); putc_('\n');
          puts_("        esi="); hx(ctx.Esi); puts_(" edi="); hx(ctx.Edi);
          puts_(" ebp="); hx(ctx.Ebp); puts_(" esp="); hx(ctx.Esp); putc_('\n');
          { DWORD r2; ReadProcessMemory(g_proc,(LPVOID)ctx.Esp,membuf,32,&r2);
            puts_("        stk:"); for(i=0;i<8;i++){ putc_(' '); hx(((DWORD*)membuf)[i]); } putc_('\n'); }
          flush();
          ctx.Dr6=0; ctx.ContextFlags=CTX_ALL; SetThreadContext(hth,&ctx);
        } else {
          for(i=0;i<npend;i++){ if(pend_tid[i]==de.tid){ plant(g_proc,pend_addr[i],0xCC); pend_tid[i]=pend_tid[npend-1]; pend_addr[i]=pend_addr[npend-1]; npend--; break; } }
        }
        CloseHandle(hth);
      } else if(ecode==EXC_BP){
        /* loader breakpoint: swallow */
      } else {
        status=DBG_EXCEPTION_NOT_HANDLED;
      }
    } else if(de.code==5){ /* EXIT_PROCESS */
      if(de.pid==pi.pid){ ContinueDebugEvent(de.pid,de.tid,status); break; }
    }
    ContinueDebugEvent(de.pid,de.tid,status);
  }
  puts_("done. hit counts:"); for(i=0;i<nbp;i++){ putc_(' '); hex_(hits[i],1); } putc_('\n');
  if(nwp>0){ puts_("wp hit counts:"); for(i=0;i<nwp;i++){ putc_(' '); hex_(wphits[i],1); } putc_('\n'); }
  flush(); CloseHandle(g_out); ExitProcess(0);
}
