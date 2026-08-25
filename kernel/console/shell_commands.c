#include "shell_internal.h"
#include "cortex_package.h"
#include "virtualbox.h"
#include "console.h"
#include "keyboard.h"
#include "vfs.h"
#include "lcp.h"
#include "openrc.h"
#include "boot.h"
#include "gui.h"
#include "asm.h"
#include "calc_app.h"
#include "mouse.h"
#include "kstring.h"
#include "timer.h"
#include "fs.h"
#include "pci.h"
#include "games.h"
#include "pmm.h"
#include "vmm.h"
#include "apps.h"
#include "process.h"
#include "serial.h"
#include "persist.h"
#include "net.h"
#include "installer.h"
#include "rust_ffi.h"
#include <stdbool.h>
#include <stdint.h>

typedef void (*cmd_func)(const char *rest, const char *arg1, const char *arg2);

static void cmd_help(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a1; (void)a2; print_help(); }
static void cmd_clear(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a1; (void)a2; console_clear(); }
static void cmd_pwd(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a1; (void)a2; console_print_color(vfs_pwd(), VGA_ATTR(VGA_LIGHT_BLUE, VGA_BLACK)); console_print("\n"); }
static void cmd_cd(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a2; if (a1[0]=='\0') console_print("Usage: cd <path>\n"); else if (!vfs_cd(a1)) console_print_color("cd: directory not found\n", VGA_ATTR(VGA_RED, VGA_BLACK)); }
static void cmd_ls(const char *rest, const char *a1, const char *a2) { (void)a1; (void)a2; bool lng=false; const char *path=0; const char *r=rest; char tok[32]; while(*r){r=parse_token(r,tok,sizeof(tok)); if(kstreq(tok,"-l")) lng=true; else if(tok[0]) path=tok;} if(lng) ls_long(path); else ls_short(path); }
static void cmd_tree(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a2; const char *t=a1[0]?a1:"/"; if(!vfs_is_dir(t)){console_print_color("tree: not a directory\n",VGA_ATTR(VGA_RED,VGA_BLACK));return;} console_print_color(t,VGA_ATTR(VGA_CYAN,VGA_BLACK)); console_print("\n"); tree_print(t,"",1,0); }
static void cmd_mkdir(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a2; if(a1[0]=='\0') console_print("Usage: mkdir <path>\n"); else if(!vfs_mkdir(a1)) console_print_color("mkdir: cannot create directory\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_rmdir(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a2; if(a1[0]=='\0') console_print("Usage: rmdir <path>\n"); else if(!vfs_rmdir(a1)) console_print_color("rmdir: cannot remove directory\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_rm(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a2; if(a1[0]=='\0') console_print("Usage: rm <path>\n"); else if(!vfs_remove(a1)) console_print_color("rm: cannot remove file\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_touch(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a2; if(a1[0]=='\0') console_print("Usage: touch <path>\n"); else if(!vfs_touch(a1)) console_print_color("touch: cannot create file\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_cp(const char *rest, const char *a1, const char *a2) { (void)rest; if(a1[0]=='\0'||a2[0]=='\0') console_print("Usage: cp <src> <dst>\n"); else if(!vfs_cp(a1,a2)) console_print_color("cp: copy failed\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_mv(const char *rest, const char *a1, const char *a2) { (void)rest; if(a1[0]=='\0'||a2[0]=='\0') console_print("Usage: mv <src> <dst>\n"); else if(!vfs_mv(a1,a2)) console_print_color("mv: move failed\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_cat(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a2; if(a1[0]=='\0') console_print("Usage: cat <file>\n"); else if(!vfs_cat(a1)) console_print_color("cat: file not found or is a directory\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_grep(const char *rest, const char *a1, const char *a2) { (void)rest; if(a1[0]=='\0'||a2[0]=='\0') console_print("Usage: grep <pattern> <file>\n"); else if(!grep_file(a2,a1)) console_print_color("grep: pattern not found\n",VGA_ATTR(VGA_RED,VGA_BLACK)); }
static void cmd_echo(const char *rest, const char *a1, const char *a2) { (void)a1; (void)a2; shell_echo(rest); }
static void cmd_uname(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; console_print_color("CortexOS",VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print_color(" x86_64 ",VGA_DEFAULT_ATTR);console_print_color("v3\n",VGA_ATTR(VGA_GREEN,VGA_BLACK)); }
static void cmd_whoami(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; console_print_color("cortexos\n",VGA_ATTR(VGA_GREEN,VGA_BLACK)); }
static void cmd_chmod(const char *rest, const char *a1, const char *a2) { (void)rest; if(a1[0]=='\0'||a2[0]=='\0') console_print("Usage: chmod <mode> <file>\n"); else console_print("chmod: permissions changed (simulated)\n"); }
static void cmd_df(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; console_print_color("Filesystem  ",VGA_ATTR(VGA_YELLOW,VGA_BLACK));console_print_color("1K-blocks  ",VGA_ATTR(VGA_YELLOW,VGA_BLACK));console_print_color("Used  ",VGA_ATTR(VGA_YELLOW,VGA_BLACK));console_print_color("Avail  ",VGA_ATTR(VGA_YELLOW,VGA_BLACK));console_print_color("Mounted on\n",VGA_ATTR(VGA_YELLOW,VGA_BLACK));console_print_color("rootfs      64        8     56     /\n",VGA_ATTR(VGA_LIGHT_GREY,VGA_BLACK)); }
static void cmd_stat(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a2; if(a1[0]=='\0') console_print("Usage: stat <file>\n"); else if(!vfs_exists(a1)){console_print_color("stat: ",VGA_ATTR(VGA_RED,VGA_BLACK));console_print(a1);console_print(": no such file or directory\n");}else vfs_stat(a1); }
static void cmd_reboot(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; console_print("Rebooting...\n"); reboot_cpu(); }
static void cmd_panic(const char *rest, const char *a1, const char *a2) { (void)a1;(void)a2; kernel_panic(rest[0]?rest:"Kernel panic triggered from shell."); }
static void cmd_lcp(const char *rest, const char *a1, const char *a2) { (void)a1;(void)a2; lcp_handle_command(rest); }
static void cmd_openrc(const char *rest, const char *a1, const char *a2) { (void)a1;(void)a2; openrc_handle_command(rest); }
static void cmd_bootctl(const char *rest, const char *a1, const char *a2) { (void)a1;(void)a2; boot_handle_command(rest); }
static void cmd_gui(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; gui_show(); }
static void cmd_asm(const char *rest, const char *a1, const char *a2) { (void)a1;(void)a2; handle_asm_command(rest); }
static void cmd_calc(const char *rest, const char *a1, const char *a2) { (void)a1;(void)a2; calc_app(rest); }
static void cmd_mouse(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; show_mouse_state(); }
static void cmd_games(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; games_menu(); }
static void cmd_fastfetch(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; fastfetch(); }
static void cmd_nano(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a2; app_nano(a1[0]?a1:0); }
static void cmd_hexview(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a2; app_hexview(a1[0]?a1:0); }
static void cmd_fm(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; app_filemanager(); }
static void cmd_htop(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; app_htop(); }
static void cmd_calc_tui(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; app_calc_tui(); }
static void cmd_ps(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; process_list(); }
static void cmd_yield(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; process_schedule(); }
static void cmd_uptime(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; unsigned long t=timer_get_ticks();unsigned long s=t/100,m=s/60,h=m/60;char b[16];kitoa(h,b,sizeof(b));console_print(b);console_print(":");kitoa(m%60,b,sizeof(b));if(kstrlen(b)<2)console_print("0");console_print(b);console_print(":");kitoa(s%60,b,sizeof(b));if(kstrlen(b)<2)console_print("0");console_print(b);console_print("\n"); }
static void cmd_install(const char *rest, const char *a1, const char *a2) { (void)a1;(void)a2; handle_install_command(rest); }
static void cmd_snake(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_snake(); }
static void cmd_tetris(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_tetris(); }
static void cmd_pong(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_pong(); }
static void cmd_2048(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_2048(); }
static void cmd_ttt(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_ttt(); }
static void cmd_minesweeper(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_minesweeper(); }
static void cmd_breakout(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_breakout(); }
static void cmd_memory(const char *rest, const char *a1, const char *a2) { (void)rest;(void)a1;(void)a2; game_launch_memory(); }

static void cmd_meminfo(const char *rest, const char *a1, const char *a2) {
	(void)rest;(void)a1;(void)a2;
	console_print_color("\n  Physical Memory\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	char buf[32]; unsigned long total=pmm_get_total_bytes(); unsigned long free=pmm_get_free_bytes();
	console_print("  Total:     "); kitoa(total/1024,buf,sizeof(buf)); console_print_color(buf,VGA_ATTR(VGA_WHITE,VGA_BLACK));
	console_print(" KB ("); kitoa(pmm_get_total_pages(),buf,sizeof(buf)); console_print(buf); console_print(" pages)\n");
	console_print("  Free:      "); kitoa(free/1024,buf,sizeof(buf)); console_print_color(buf,VGA_ATTR(VGA_LIGHT_GREEN,VGA_BLACK));
	console_print(" KB ("); kitoa(pmm_get_free_pages(),buf,sizeof(buf)); console_print(buf); console_print(" pages)\n");
	console_print("  Used:      "); kitoa((total-free)/1024,buf,sizeof(buf)); console_print_color(buf,VGA_ATTR(VGA_LIGHT_RED,VGA_BLACK));
	console_print(" KB ("); kitoa(pmm_get_used_pages(),buf,sizeof(buf)); console_print(buf); console_print(" pages)\n");
	console_print_color("\n  Virtual Memory\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print("  Status:    "); unsigned long cr0; __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
	if(cr0&0x80000000) console_print_color("Active (paging enabled)\n",VGA_ATTR(VGA_LIGHT_GREEN,VGA_BLACK));
	else console_print_color("Inactive\n",VGA_ATTR(VGA_RED,VGA_BLACK));
	console_print("  Page Size: 4096 bytes\n"); console_print("  Pages:     ");
	kitoa(pmm_get_total_pages(),buf,sizeof(buf)); console_print(buf); console_print(" mapped (identity)\n"); console_print("\n");
}

static void cmd_lspci(const char *rest, const char *a1, const char *a2) {
	(void)rest;(void)a1;(void)a2; int cnt=pci_get_device_count();
	if(!cnt){console_print("No PCI devices found.\n");return;}
	console_print_color("BDF    Vendor  Device  Class\n",VGA_ATTR(VGA_YELLOW,VGA_BLACK));
	for(int i=0;i<cnt;i++){const struct pci_device*d=pci_get_device(i);if(!d)break;
		char bdf[32],num[8];kitoa(d->bus,num,sizeof(num));kstrcpy(bdf,num,sizeof(bdf));kstrcat(bdf,":",sizeof(bdf));
		kitoa(d->device,num,sizeof(num));kstrcat(bdf,num,sizeof(bdf));kstrcat(bdf,".",sizeof(bdf));
		kitoa(d->function,num,sizeof(num));kstrcat(bdf,num,sizeof(bdf));
		int len=kstrlen(bdf);while(len<7){bdf[len++]=' ';bdf[len]='\0';}console_print(bdf);
		char vd[12];kitoa(d->vendor_id,num,sizeof(num));kstrcpy(vd,num,sizeof(vd));kstrcat(vd,":",sizeof(vd));
		kitoa(d->device_id,num,sizeof(num));kstrcat(vd,num,sizeof(vd));
		len=kstrlen(vd);while(len<10){vd[len++]=' ';vd[len]='\0';}console_print(vd);
		console_print("0x");char cls[8];kitoa(d->class_code,num,sizeof(num));kstrcpy(cls,num,sizeof(cls));
		kstrcat(cls,".",sizeof(cls));kitoa(d->subclass,num,sizeof(num));kstrcat(cls,num,sizeof(cls));
		console_print(cls);console_print("\n");
	}
}

static void cmd_kblayout(const char *rest, const char *a1, const char *a2) {
	(void)rest;(void)a2; if(a1[0]=='\0'){int cur=keyboard_get_layout();console_print("Current layout: ");console_print(layout_name(cur));console_print("\nUsage: kblayout us|es|de\n");return;}
	int id;if(kstreq(a1,"us"))id=KB_LAYOUT_US;else if(kstreq(a1,"es"))id=KB_LAYOUT_ES;else if(kstreq(a1,"de"))id=KB_LAYOUT_DE;
	else{console_print("Unknown layout. Use us, es, de\n");return;}
	if(keyboard_set_layout(id)==0){console_print("Layout set to ");console_print(layout_name(id));console_print("\n");}
}

static void cmd_hexdump(const char *rest, const char *a1, const char *a2) {
	(void)rest;(void)a2; if(a1[0]=='\0'){console_print("Usage: hexdump <file>\n");return;}
	const void*data=vfs_read(a1);size_t sz=vfs_get_size(a1);
	if(!data||sz==0){console_print_color("hexdump: file not found or empty\n",VGA_ATTR(VGA_RED,VGA_BLACK));return;}
	const unsigned char*p=(const unsigned char*)data;
	for(size_t i=0;i<sz;i+=16){char addr[16];kxtoa(i,addr,sizeof(addr));int alen=kstrlen(addr);while(alen<8){console_print("0");alen++;}console_print(addr);console_print("  ");
		char hex[4];for(int j=0;j<16&&i+j<sz;++j){kxtoa(p[i+j],hex,sizeof(hex));if(kstrlen(hex)<2)console_print("0");console_print(hex);console_print(" ");}
		console_print(" |");for(int j=0;j<16&&i+j<sz;++j){char c=p[i+j];console_putchar((c>=32&&c<127)?c:'.');}console_print("|\n");}
}

static void cmd_wc(const char *rest, const char *a1, const char *a2) {
	(void)rest;(void)a2; if(a1[0]=='\0'){console_print("Usage: wc <file>\n");return;}
	const void*data=vfs_read(a1);size_t sz=vfs_get_size(a1);
	if(!data||sz==0){console_print_color("wc: file not found\n",VGA_ATTR(VGA_RED,VGA_BLACK));return;}
	const char*t=(const char*)data;size_t lines=0,words=0,chars=sz;int inw=0;
	for(size_t i=0;i<sz;i++){if(t[i]=='\n')lines++;if(t[i]==' '||t[i]=='\n'||t[i]=='\t'){if(inw){words++;inw=0;}}else inw=1;}
        if(inw)words++;
	char buf[16];kitoa(lines,buf,sizeof(buf));console_print(buf);console_print(" ");
	kitoa(words,buf,sizeof(buf));console_print(buf);console_print(" ");kitoa(chars,buf,sizeof(buf));
	console_print(buf);console_print(" ");console_print_color(a1,VGA_ATTR(VGA_LIGHT_BLUE,VGA_BLACK));console_print("\n");
}

static void cmd_head(const char *rest, const char *a1, const char *a2) {
	if(a1[0]=='\0'){console_print("Usage: head [-n N] <file>\n");return;}
	int n=10;const char*path=a1;if(kstreq(a1,"-n")){n=parse_int(a2);if(n<=0)n=10;char tok[128];const char*r=rest;r=parse_token(r,tok,sizeof(tok));r=parse_token(r,tok,sizeof(tok));path=kskip_spaces(r);}
	const void*data=vfs_read(path);size_t sz=vfs_get_size(path);if(!data||sz==0){console_print_color("head: file not found\n",VGA_ATTR(VGA_RED,VGA_BLACK));return;}
	const char*t=(const char*)data;int shown=0;for(size_t i=0;i<sz&&shown<n;i++){console_putchar(t[i]);if(t[i]=='\n')shown++;}if(!shown)console_print("\n");
}

static void cmd_tail(const char *rest, const char *a1, const char *a2) {
	if(a1[0]=='\0'){console_print("Usage: tail [-n N] <file>\n");return;}
	int n=10;const char*path=a1;if(kstreq(a1,"-n")){n=parse_int(a2);if(n<=0)n=10;char tok[128];const char*r=rest;r=parse_token(r,tok,sizeof(tok));r=parse_token(r,tok,sizeof(tok));path=kskip_spaces(r);}
	const void*data=vfs_read(path);size_t sz=vfs_get_size(path);if(!data||sz==0){console_print_color("tail: file not found\n",VGA_ATTR(VGA_RED,VGA_BLACK));return;}
	const char*t=(const char*)data;int total=0;for(size_t i=0;i<sz;i++)if(t[i]=='\n')total++;int skip=total-n;if(skip<0)skip=0;
	int line=0;for(size_t i=0;i<sz;i++){if(line>=skip)console_putchar(t[i]);if(t[i]=='\n')line++;}if(sz>0&&t[sz-1]!='\n')console_print("\n");
}

static void cmd_sudo(const char *rest, const char *a1, const char *a2) {
	(void)a1;(void)a2; if(rest[0]=='\0'){console_print("Usage: sudo <command>\n");return;}
	char s_cmd[32],s_arg1[128],s_rest[192];const char*sp=rest;sp=parse_token(sp,s_cmd,sizeof(s_cmd));const char*s_tail=sp;sp=parse_token(sp,s_arg1,sizeof(s_arg1));copy_rest(s_tail,s_rest,sizeof(s_rest));
	if(kstreq(s_cmd,"ls")){bool lng=false;const char*path=0;const char*r=s_rest;char tok[32];while(*r){r=parse_token(r,tok,sizeof(tok));if(kstreq(tok,"-l"))lng=true;else if(tok[0])path=tok;}if(lng)ls_long(path);else ls_short(path);}
	else if(kstreq(s_cmd,"reboot")){console_print("Rebooting (sudo)...\n");reboot_cpu();}
	else if(kstreq(s_cmd,"poweroff")||kstreq(s_cmd,"shutdown")){console_print("Shutting down...\n");halt_cpu();}
	else{console_print_color("[sudo] ",VGA_ATTR(VGA_YELLOW,VGA_BLACK));console_print_color(s_cmd,VGA_ATTR(VGA_WHITE,VGA_BLACK));console_print(": command executed as root\n");}
}

static void cmd_proc(const char *rest, const char *a1, const char *a2) {
	(void)rest; if(a1[0]=='\0'){console_print("Usage: proc create|kill|list <name>\n");return;}
	if(kstreq(a1,"create")){int pid=process_create(a2[0]?a2:"user",0,true);if(pid>0){char buf[16];kitoa(pid,buf,sizeof(buf));console_print("Created process PID=");console_print(buf);console_print("\n");}else console_print_color("Failed to create process\n",VGA_ATTR(VGA_RED,VGA_BLACK));}
	else if(kstreq(a1,"list"))process_list();
	else if(kstreq(a1,"kill")){if(a2[0]=='\0')console_print("Usage: proc kill <pid>\n");else{int pid=parse_int(a2);struct process*p=process_current();if(p&&p->pid==pid)console_print("Cannot kill current process\n");else process_exit(0);}}
}

static void cmd_serial(const char *rest, const char *a1, const char *a2) {
	(void)rest;(void)a2; if(a1[0]=='\0'){console_print("Usage: serial send|recv|status\n");return;}
	if(kstreq(a1,"status"))console_print_color("Serial: COM1 @ 115200 baud\n",VGA_ATTR(VGA_LIGHT_GREEN,VGA_BLACK));
	else if(kstreq(a1,"send")){const char*m=rest;while(*m==' ')m++;if(*m){serial_print(m);serial_print("\n");console_print_color("Sent via serial\n",VGA_ATTR(VGA_LIGHT_GREEN,VGA_BLACK));}}
	else if(kstreq(a1,"recv")){if(serial_available()){char c=serial_getchar();console_putchar(c);console_print("\n");}else console_print("No data available\n");}
}

static void cmd_persist(const char *rest, const char *a1, const char *a2) {
	(void)rest; if(a1[0]=='\0'){console_print("Usage: persist save|load|list|delete <name>\n");return;}
	if(kstreq(a1,"save")){
		if(a2[0]=='\0'){console_print("Usage: persist save <file> <text>\n");return;}
		const char*t=rest;while(*t&&*t!=' ')t++;while(*t==' ')t++;while(*t&&*t!=' ')t++;while(*t==' ')t++;
		if(*t){persist_save_file(a2,t,kstrlen(t));console_print_color("Saved\n",VGA_ATTR(VGA_LIGHT_GREEN,VGA_BLACK));}
	} else if(kstreq(a1,"load")){
		if(a2[0]=='\0'){console_print("Usage: persist load <name>\n");return;}
		char buf[256];uint32_t sz;if(persist_load_file(a2,buf,sizeof(buf),&sz)){for(uint32_t i=0;i<sz;i++)console_putchar(buf[i]);console_print("\n");}
		else console_print_color("File not found\n",VGA_ATTR(VGA_RED,VGA_BLACK));
	} else if(kstreq(a1,"list")){console_print("Persistent files: ");char buf[16];kitoa(persist_get_total_size(),buf,sizeof(buf));console_print(buf);console_print(" bytes stored\n");}
	else if(kstreq(a1,"delete")){if(a2[0]=='\0')console_print("Usage: persist delete <name>\n");else{if(persist_delete_file(a2))console_print_color("Deleted\n",VGA_ATTR(VGA_LIGHT_GREEN,VGA_BLACK));else console_print_color("Not found\n",VGA_ATTR(VGA_RED,VGA_BLACK));}}
}

static void cmd_net(const char *rest, const char *a1, const char *a2) {
	(void)rest;(void)a2; if(a1[0]=='\0'){
		console_print_color("\n-- Network Interfaces --\n",VGA_ATTR(VGA_YELLOW,VGA_BLACK));int cnt=net_get_interface_count();
		for(int i=0;i<cnt;i++){const struct net_interface*iface=net_get_interface(i);if(!iface)continue;
			console_print("  ");console_print_color(iface->name,VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print("  MAC: ");char buf[8];
			for(int j=0;j<6;j++){kxtoa(iface->mac[j],buf,sizeof(buf));if(kstrlen(buf)<2)console_print("0");console_print(buf);if(j<5)console_print(":");}
			console_print("  IP: ");kitoa(iface->ip[0],buf,sizeof(buf));console_print(buf);console_print(".");kitoa(iface->ip[1],buf,sizeof(buf));console_print(buf);
			console_print(".");kitoa(iface->ip[2],buf,sizeof(buf));console_print(buf);console_print(".");kitoa(iface->ip[3],buf,sizeof(buf));console_print(buf);
			console_print(iface->up?" [UP]":" [DOWN]");console_print("\n");
		}
	} else if(kstreq(a1,"ping")){console_print("PING 127.0.0.1: ");uint8_t pkt[64];for(int i=0;i<64;i++)pkt[i]=0;pkt[0]=0x08;pkt[1]=0x00;net_send_packet(0,pkt,64);console_print_color("64 bytes transmitted\n",VGA_ATTR(VGA_LIGHT_GREEN,VGA_BLACK));}
	else if(kstreq(a1,"setip")){if(a2[0]=='\0')console_print("Usage: net setip <iface> <ip>\n");else console_print("IP set (simulated)\n");}
}

static void cmd_rust(const char *rest, const char *a1, const char *a2) {
	if(a1[0]=='\0'||kstreq(a1,"info")){rust_info();return;}
	char buf[32],nb[16],tok[32];const char*p;
	if(kstreq(a1,"fib")){int n=katoi(a2);if(n>=0){kutoa(rust_fibonacci(n),buf,sizeof(buf));console_print("fib(");console_print(a2);console_print(") = ");console_print_color(buf,VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print("\n");}}
	else if(kstreq(a1,"fact")){int n=katoi(a2);if(n>=0){kutoa(rust_factorial(n),buf,sizeof(buf));console_print("fact(");console_print(a2);console_print(") = ");console_print_color(buf,VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print("\n");}}
	else if(kstreq(a1,"prime")){int n=katoi(a2);if(n>=0){console_print(a2);console_print(rust_is_prime(n)?" is prime":" is not prime");console_print("\n");}}
	else if(kstreq(a1,"gcd")){int x=katoi(a2);p=rest;while(*p&&*p!=' ')p++;while(*p==' ')p++;p=parse_token(p,tok,sizeof(tok));int y=katoi(tok);kutoa(rust_gcd(x,y),buf,sizeof(buf));console_print("gcd(");console_print(a2);console_print(",");console_print(tok);console_print(") = ");console_print_color(buf,VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print("\n");}
	else if(kstreq(a1,"add")){int x=katoi(a2);p=rest;while(*p&&*p!=' ')p++;while(*p==' ')p++;p=parse_token(p,tok,sizeof(tok));int y=katoi(tok);kitoa(rust_add(x,y),buf,sizeof(buf));kitoa(x,nb,sizeof(nb));console_print(nb);console_print(" + ");kitoa(y,nb,sizeof(nb));console_print(nb);console_print(" = ");console_print_color(buf,VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print("\n");}
	else if(kstreq(a1,"div")){int x=katoi(a2);p=rest;while(*p&&*p!=' ')p++;while(*p==' ')p++;p=parse_token(p,tok,sizeof(tok));int y=katoi(tok);kitoa(rust_div(x,y),buf,sizeof(buf));kitoa(x,nb,sizeof(nb));console_print(nb);console_print(" / ");kitoa(y,nb,sizeof(nb));console_print(nb);console_print(" = ");console_print_color(buf,VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print("\n");}
	else if(kstreq(a1,"swap16")){unsigned int x=(unsigned int)katoi(a2);kutoa(rust_swap_bytes16((unsigned short)x),buf,sizeof(buf));console_print(buf);console_print("\n");}
	else if(kstreq(a1,"swap32")){unsigned int x=(unsigned int)katoi(a2);kutoa(rust_swap_bytes32(x),buf,sizeof(buf));console_print(buf);console_print("\n");}
	else if(kstreq(a1,"popcount")){unsigned long x=(unsigned long)katoi(a2);kutoa(rust_popcount(x),buf,sizeof(buf));console_print(buf);console_print("\n");}
	else if(kstreq(a1,"leading")){unsigned long x=(unsigned long)katoi(a2);kutoa(rust_leading_zeros(x),buf,sizeof(buf));console_print(buf);console_print("\n");}
	else if(kstreq(a1,"trailing")){unsigned long x=(unsigned long)katoi(a2);kutoa(rust_trailing_zeros(x),buf,sizeof(buf));console_print(buf);console_print("\n");}
	else if(kstreq(a1,"strlen")){kutoa(rust_strlen((const unsigned char*)a2),buf,sizeof(buf));console_print("strlen(\"");console_print(a2);console_print("\") = ");console_print_color(buf,VGA_ATTR(VGA_CYAN,VGA_BLACK));console_print("\n");}
	else if(kstreq(a1,"toupper")){unsigned char tmp[128];kstrcpy((char*)tmp,a2,sizeof(tmp));rust_to_uppercase(tmp,kstrlen((const char*)tmp));console_print((const char*)tmp);console_print("\n");}
	else if(kstreq(a1,"tolower")){unsigned char tmp[128];kstrcpy((char*)tmp,a2,sizeof(tmp));rust_to_lowercase(tmp,kstrlen((const char*)tmp));console_print((const char*)tmp);console_print("\n");}
	else{console_print("Rust subcommands: info, fib <n>, fact <n>, prime <n>, gcd <a> <b>, add <a> <b>, div <a> <b>, swap16 <n>, swap32 <n>, popcount <n>, leading <n>, trailing <n>, strlen <s>, toupper <s>, tolower <s>\n");}
}

static void cmd_vbox(const char *rest, const char *a1, const char *a2) { (void)rest; (void)a1; (void)a2; virtualbox_print_info(); }

static void cmd_pkg(const char *rest, const char *a1, const char *a2);

static const struct { const char *name; cmd_func func; } cmd_table[] = {
	{"help", cmd_help}, {"clear", cmd_clear}, {"pwd", cmd_pwd}, {"cd", cmd_cd},
	{"ls", cmd_ls}, {"tree", cmd_tree}, {"mkdir", cmd_mkdir}, {"rmdir", cmd_rmdir},
	{"rm", cmd_rm}, {"touch", cmd_touch}, {"cp", cmd_cp}, {"mv", cmd_mv},
	{"cat", cmd_cat}, {"grep", cmd_grep}, {"echo", cmd_echo},
	{"uname", cmd_uname}, {"whoami", cmd_whoami}, {"meminfo", cmd_meminfo},
	{"sudo", cmd_sudo}, {"chmod", cmd_chmod}, {"df", cmd_df}, {"stat", cmd_stat},
	{"reboot", cmd_reboot}, {"panic", cmd_panic},
	{"lcp", cmd_lcp}, {"openrc", cmd_openrc}, {"bootctl", cmd_bootctl},
	{"gui", cmd_gui}, {"lspci", cmd_lspci}, {"asm", cmd_asm},
	{"calc", cmd_calc}, {"mouse", cmd_mouse}, {"games", cmd_games},
	{"fastfetch", cmd_fastfetch}, {"neofetch", cmd_fastfetch}, {"sysinfo", cmd_fastfetch},
	{"kblayout", cmd_kblayout}, {"hexdump", cmd_hexdump}, {"wc", cmd_wc},
	{"head", cmd_head}, {"tail", cmd_tail},
	{"snake", cmd_snake}, {"tetris", cmd_tetris}, {"pong", cmd_pong},
	{"2048", cmd_2048}, {"ttt", cmd_ttt}, {"minesweeper", cmd_minesweeper},
	{"breakout", cmd_breakout}, {"memory", cmd_memory},
	{"nano", cmd_nano}, {"hexview", cmd_hexview},
	{"fm", cmd_fm}, {"filemanager", cmd_fm},
	{"htop", cmd_htop}, {"calc-tui", cmd_calc_tui},
	{"ps", cmd_ps}, {"proc", cmd_proc}, {"yield", cmd_yield},
	{"serial", cmd_serial}, {"persist", cmd_persist}, {"net", cmd_net},
	{"uptime", cmd_uptime}, {"install", cmd_install},
	{"rust", cmd_rust}, {"vbox", cmd_vbox}, {"pkg", cmd_pkg}, {0, 0}
};

static void cmd_pkg(const char *rest, const char *a1, const char *a2)
{
	(void)rest; (void)a2;
	if (a1[0] == '\0' || !kstreq(a1, "run")) { cortex_package_help(); return; }
	/* The package path is the second token in the command line. */
	{ char path[128]; const char *p = rest; p = parse_token(p, path, sizeof(path));
		if (path[0] == '\0' || cortex_package_run(path) < 0)
			console_print_color("pkg: invalid package or command failed\n", VGA_ATTR(VGA_RED, VGA_BLACK)); }
}

int shell_execute_line(const char *line)
{
	char cmd[32], arg1[128], arg2[128], rest[192];
	const char *p = line, *tail;
	p = parse_token(p, cmd, sizeof(cmd)); tail = p;
	p = parse_token(p, arg1, sizeof(arg1)); p = parse_token(p, arg2, sizeof(arg2));
	copy_rest(tail, rest, sizeof(rest));
	for (int i = 0; cmd_table[i].name; ++i)
		if (kstreq(cmd, cmd_table[i].name)) { cmd_table[i].func(rest, arg1, arg2); return 0; }
	return -1;
}

void shell_run(void)
{
	char buf[256];

	console_print_color("+==========================================+\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("|   _   _            _   _  ____           |\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("|  | \\ | | ___  _ __| | | |/ ___|         |\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("|  |  \\| |/ _ \\| '__| | | | |  _          |\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("|  | |\\  | (_) | |  | |_| | |_| |         |\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("|  |_| \\_|\\___/|_|   \\___/ \\____|  v3     |\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("|                                          |\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("|   Type 'help' for available commands     |\n", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	console_print_color("+==========================================+\n\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));

	while (1) {
		print_prompt();
		int n = keyboard_readline(buf, sizeof(buf));
		if (n == 0)
			continue;
		char cmd[32];
		char arg1[128];
		char arg2[128];
		char rest[192];
		const char *p = buf;
		p = parse_token(p, cmd, sizeof(cmd));
		const char *tail = p;
		p = parse_token(p, arg1, sizeof(arg1));
		p = parse_token(p, arg2, sizeof(arg2));
		copy_rest(tail, rest, sizeof(rest));

		int found = (shell_execute_line(buf) == 0);
		if (!found) {
			console_print_color("cortexos: command not found: ", VGA_ATTR(VGA_RED, VGA_BLACK));
			console_print_color(cmd, VGA_ATTR(VGA_WHITE, VGA_BLACK));
			console_print("\n");
		}
	}
}
