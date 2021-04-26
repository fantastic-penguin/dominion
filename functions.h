#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

// Forward declare... lots of stuff.
void init_screen();
void online_info();
void read_races();
void read_world(Sworld* wp, char fname[]);
void print_nations();
void usageerr(int argc, char *argv[]);
void clean_exit();
void init();
void init_user(int innation, char nation[]);
void init_keymap();
void init_screen();
void intro(Sworld *wp, Snation *np);
void noncritical();
void main_loop();
void cleanup();
void clean_exit();
void statline(char s1[], char s2[]);
void draw_map();
void set_cursor();
int re_center(int x, int y);
void wrap(Pt *pp);
int army_is_in_sector(Ssector *sp, int owner, int id);
int first_sect_army(Ssector *sp);
void gen_exec(char *s);
void del_lock(int id);
int is_active_ntn(Snation* np);
int get_diplo_status(Sdiplo **dm, int nation1, int nation2);
int getexec(FILE *fp, struct argument args[]);
void load_options(Snation* np);
void run_exec_line(Snation *np, struct argument args[]);
int parse_exec_line(char line[], struct argument args[]);
int insert_army_nation(Snation *np, Sarmy *ap, int chosen_id);
int insert_army_sector(Ssector *sp, Sarmy *ap);
void delete_army_nation(Snation *np, Sarmy *ap);
void delete_army_sector(Ssector *sp, Sarmy *ap);
Sarmy *get_army(Snation *np, int id);
void addsector(Snation *np, int x, int y);
int basic_move_rate(Snation *np);
int is_spirit(Sarmy *ap);
int destroy_nation(int id);
void load_army_types();
void load_spirit_types();
void load_master_execs();
int get_nation_id(char name[]);
void get_crypt_pass(char prompt[], char pass[], WINDOW *w, char def_pass[]);
void handle_locks(int id);
void get_avail_armies(Suser *up, int skill);
void get_spells(Suser *up, int skill);
void get_spirits(Suser *up, int skill);
void load_nation(int id, Snation *np);
void read_in_diplo(Sdiplo **dm, int n);
void load_dead_hspells(Suser *up, int flag);
void load_h_spells(Suser *up);
void find_visible_sectors(int **visible_sectors);
void ask_for_mail_reader(WINDOW *win);
void save_options(Snation *np);
void touch_all_wins();
void dom_getline(char s[], int n);
int wget_string(WINDOW * w, char * rets, int len);
void mem_error();
int is_master_lock();
void set_lock(int id);

#endif // _FUNCTIONS_H_
