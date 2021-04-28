#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "news.h"

// Forward declare... lots of stuff.
int mygetch();
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
void statline2(char s1[], char s2[]);
void statline2_err(char s1[], char s2[]);
void statline_prompt(char s1[], char s2[]);
void statline2_prompt(char s1[], char s2[]);
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
void change_dip_status(Sdiplo **dm, Sdiplo **initial_dm, int n1, int n2);
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
void destroy_nation(int id);
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
void load_dead_hspells(Suser *up, int flag);
void load_h_spells(Suser *up);
void find_visible_sectors(int **visible_sectors);
void ask_for_mail_reader(WINDOW *win);
void save_options(Snation *np);
void touch_all_wins();
void dom_getline(char s[], int n);
int wget_string(WINDOW *w, char *rets, int len);
int wget_number(WINDOW *w, int *num);
int wget_name(WINDOW *w, char *name);
void mem_error();
int is_master_lock();
void set_lock(int id);
int check_compressed_world(char fname[]);
void read_nation(Snation *np, FILE *fp, Sworld *wp);
void write_nation(Snation *np, FILE *fp);
void init_wrap();
void critical();
void init_options(Snation *np);
int get_n_civil(Snation *np);
int get_n_soldiers(Snation *np);
int get_n_priests(Snation *np);
int get_n_students(Snation *np);
int get_generic_move_cost(Snation* np, Ssector* sp);
int get_army_move_cost(Snation* np, Ssector* sp, Sarmy *ap);
int n_workers(Ssector *sp);
int xrel(int x, int y, Pt cap);
int yrel(int x, int y, Pt cap);
int map_alt(int altitude);
int has_visible_army(Ssector *sp, Suser *up);
void draw_budget_screen(WINDOW *w, Snation * np);
void storage_budget(Snation *np);
void do_tech_budget(WINDOW *w, Snation * np);
void do_magic_budget(WINDOW *w, Snation * np);
void dump_current_screen();
int calc_revenue(Snation *np);
int calc_serv_revenue(Snation *np);
int calc_expend(Snation *np);
int calc_jewels(Snation *np);
int calc_food(Snation *np);
int calc_expend_jewels(Snation *np);
int calc_metal(Snation *np);
int calc_expend_metal(Snation *np);
int calc_expend_food(Snation *np);
int military_maint(Snation *np);
int military_maint_metal(Snation *np);
int military_maint_jewels(Snation *np);
int non_profit_maint(Snation *np);
void draw_storage_budget(WINDOW *w, Snation * np);
void start_help_win();
void show_help();
void end_help_win();
int sect_n_armies(Ssector *sp);
int sector_metal(Ssector *sp);
int sector_jewels(Ssector *sp);
int sector_food(Ssector *sp);
void name_sector(WINDOW *w);
int redesignate(Snation *np, WINDOW *w, int confirm);
void move_capital(Snation* np, Ssector *sp);
void cmoney(Snation *np, int m);
void cmetal(Snation *np, int m);
void cjewels(Snation *np, int j);
void cspell_pts(Snation *np, int pts);
void cfood(Snation *np, int f);
void ctech_skill(Snation *np, int change);
void cmag_skill(Snation *np, int change);
int my_scr_dump(WINDOW *w, char fname[]);
void mail_read(int reader);
void mail_write();
int mail_send(char mailfile[], int sender, int receiver, char subject[]);
void edit(char *t_fn);
int has_mail_lock(int id);
void lock_mail(int nation);
void unlock_mail(int nation);
void show_file(char name[]);
void get_space();
void cinfo(char fname[], char tag[]);
void bad_key();
int first_sect_mage_id(Snation *np, Ssector *sp);
void show_armies(Ssector *sp);
void list_spells(Suser *up);
void list_spirits(Suser *up);
void summon(Suser *up, WINDOW *w);
void cast_spell(Suser *up, WINDOW *w);
void initiate_mage(Suser *up, WINDOW *w);
void show_hanging_spells(Suser *up);
int is_mage(Sarmy *ap);
int spirit_type_index(char type[]);
int exec_spell(Sspell *spellp, WINDOW *w);
void exec_summon(int type_index, char name[]);
void exec_initiate(char name[]);
int zoom_on_h_spell(Sh_spell *h_list, WINDOW *sw);
int zoom_del_h_spell(Sh_spell *h_list, WINDOW *sw);
int free_army_id(Snation *np);
void aflag_set(Sarmy *ap, int flag);
void aflag_clear(Sarmy *ap, int flag);
void fix_sector_line(char line[], char s[]);
void delete_hanging_spell(Sh_spell *sp1);
void init_diplo(int n);
int read_in_diplo(Sdiplo **dm, int n);
void read_initial_diplo(Sdiplo **dm, int n);
void increase_diplo(Sdiplo **dm_old, Sdiplo **dm_new, int n, Snation *np);
void dump_diplo(Snation *np, Sdiplo **dm, int n);
void free_diplo(Sdiplo **dm, int n);
void update_diplo();
int have_met(Sdiplo **dm, int nation1, int nation2);
int diplo_is_locked();
int my_scr_restore(char fname[]);
void draw_info_screen(WINDOW * w, Snation * np);
void cpass(Snation *np, char pass[]);
int change_passwd(Snation *np, WINDOW *w);
void change_leader(Snation *np, WINDOW *w);
void set_aggressiveness(Snation *np, WINDOW *current_win);
int univ_intel(Snation * np);
int priestliness(Snation * np);
void draw_production_screen(WINDOW * w, Snation *np);
int get_employed(Snation *np);
int get_emp_met(Snation *np);
int get_emp_jwl(Snation *np);
int get_emp_farm(Snation *np);
int get_emp_serv(Snation *np);
int get_unemployed(Snation *np);
int get_unemp_met(Snation *np);
int get_unemp_jwl(Snation *np);
int get_unemp_farm(Snation *np);
int get_unemp_serv(Snation *np);
int military_maint(Snation *np);
int military_maint_metal(Snation *np);
int military_maint_jewels(Snation *np);
int military_maint_spell_pts(Snation *np);
char info_report();
void spy_report(int id);
int spy_figure(int n, int expend, Snation *spying_np, Snation *spied_np, int cost_fact);
void swapgoods(Snation *fromnat, Snation *tonat, int amt, int item);
void get_apparent_type(Sarmy *ap, char type[]);
void get_army_status(Sarmy *ap, char s[]);
int jarmy();
void move_army(int id, WINDOW *aw);
int transport();
void previous_army();
void next_army();
int next_nation_army(Snation *np, int old_id);
int prev_nation_army(Snation *np, int old_id);
void just_moved();
void change_army_status(WINDOW *aw, int id);
void list_available_armies(Suser *up, WINDOW *aw);
int draft_army(Snation *np);
int army_disband(Ssector *sp, Sarmy *ap);
void army_examine(Sarmy * ap);
int zoom_armies(Suser *up, Ssector *sp);
int army_merge(Sarmy * ap);
int army_split(Sarmy * ap);
void donate_army(Sarmy *ap, Ssector *sp);
int next_sect_army(Ssector *sp, Sarmy *ap);
int prev_sect_army(Ssector *sp, Sarmy *ap);
int cargo_not_empty(Scargo *cargop);
int is_spelled(Sarmy *ap);
int can_occupy(Sarmy *ap);
int can_patrol(Sarmy *ap);
int can_intercept(Sarmy *ap);
int can_garrison(Sarmy *ap);
void army_visibility(int **visible_sectors, Sarmy *ap);
int is_avail_army_type(Suser *up, char type[]);
int army_cost(Sarmy *ap);
int army_cost_metal(Sarmy *ap);
int army_weight(Sarmy *ap);
void cpeople_sector(Ssector *sp, int p);
int army_type_index(char type[]);
int zoom_army_page(WINDOW *azlw, Sarmy *armies, Ssector *sp, int page, int len_page, int show_cargo);
void zoom_list_armies(WINDOW *azlw, Sarmy *armies, Ssector *sp, int start_army, int len_win, int cargo);
int get_spell_pts_maint(Sarmy *ap);
void donate_cargo(int x, int y, int from_id, int to_id, Scargo *cargo);
int army_move_rate(Snation *np, Sarmy *ap);
char *contents(int money, int metal, int jewels, int food, int people, int army, Pt *title, int sp);
int is_army(Sarmy *ap);
int is_navy(Sarmy *ap);
int is_mage(Sarmy *ap);
void add_army_type(Suser *up, char type[]);
int add_flag(char *s, int flags, int i, int has_slash);
int lock_diplo(Snation *np);
void unlock_diplo();
void handle_meet(Sdiplo **dm, int nation1, int nation2);
void get_indices(Sdiplo **dm, int nation1, int nation2, int *n1, int *n2);
int new_mag_skill(Snation *np);
int new_spell_pts(Snation *np);
void show_spell(Sspell *spellp);
void show_spirit(Sspirit *spiritp);
void draw_map_regular();
void draw_map_compact();
int is_under_sectw(int x, int y);
int which_mark(int x, int y, Suser *up);
void show_sector(int x, int y);
void insert(char *in_name, FILE *out_pntr);
char pager(char fname[]);
void group_insert(s_group **first, s_group *temp);
void post_news_file(char *news_file, char *group_name, char *subject, int id);
void cargo_statline(WINDOW *w, Suser *up);
void transport_load(Sarmy *ap, WINDOW *tw);
void transport_unload(Sarmy *ap, WINDOW *tw);
void transport_transfer(Sarmy *ap, WINDOW *tw);
int good_loading_place(Snation *np, Ssector *sp, char type, int quantity);
int good_unloading_place(Snation *np, Ssector *sp, char type, int quantity);
int load_too_big(Snation *np, Sarmy *ap, char c, int quantity);
void adjust_cargo(Sarmy *ap);
int cargo_weight(Scargo *cargo);
int caravan_weight(Sarmy *ap);
int is_trade_place(Ssector *sp);
int cargo_is_locked();
void lock_cargo();
void unlock_cargo();
int good_altitude(Ssector *sp, Snation *np);
int good_army_altitude(Snation *np, Ssector *sp, Sarmy *ap);
void add_h_spell(Sh_spell **listp, Sh_spell *h_spellp);
void write_h_spells();
void prepare_h_spell(Sh_spell *h_spellp, char name[], int nat_id, int thons_left, int n_lines);
void got_dead_h_spell(Sh_spell *sp1);
void write_dead_spell(Sh_spell *sp1);
int is_army_spell(Sh_spell *sp1);
void reset_spelled_flags();
void free_h_spell(Sh_spell *sp1);
void reset_one_spell(Sh_spell *sp1);
void root_edit_sector();
void root_edit_nation();
void root_destroy_nation(Snation *np, WINDOW *renw);
void cowner_sector(Ssector *sp, int change);
void csoil_sector(Ssector *sp, int change);
void cmetal_sector(Ssector *sp, int change);
void cjewels_sector(Ssector *sp, int change);
void caltitude_sector(Ssector *sp, int change);
void subtsector(Snation *np, int x, int y);
int is_good_order(char name[]);
int free_nation_mark(Sworld *wp, Symbol c);
char diplo_report(Snation *nation);
void interpolate(double *X[], int oldx, int oldy, int newx, int newy);
void write_world(Sworld *wp, char fname[]);
void set_update_time();
int verify_gamemaster();
void set_master_lock();
void del_master_lock();
int is_any_lock();
int find_free_nation(Sworld *wp);
void add_npcs(char *npc_fn, int npcs_get_mail);
void add_player(int n);
int unique_name(char name[]);
int get_race_mark();
void choose_mag_order(char s[]);
void add_special_mag(Snation *np, char mag_ord[]);
int setup_new_nation(char nation_name[NAMELEN], char nation_pass[PASSLEN], char leader_name[NAMELEN],
    char nation_race, Symbol nation_mark, char mag_ord[NAMELEN], int npc_flag, int npcagg, int npcexp, int npciso);
void list_mag_orders();
int isolated(int x, int y);
int sect_desire(Snation *np, int x, int y);
int default_desig(Ssector *sp);
void form_valid_mark_str(char mark_str[]);
void backuplib(char dirname[]);
void init_work_data(int xmax, int ymax);
void npc_moves(Snation *np);
void docargos();
void dotechno(Snation *np, FILE *mailfile);
void dospy(Snation *np, FILE *mailfile);
void domagic(Snation *np, FILE *mailfile);
void domoney(Snation *np);
void dometal(Snation *np);
void dojewels(Snation *np);
void dofood(Snation *np);
void docivilians(Snation *np);
void dobattles();
void take_sectors();
void reset_armies();
void send_mail();
void post_statistics();
void clear_dead_hspells();
void clear_h_spells();
void take_capital(Snation *np1, Snation *np2, FILE *mfile);
void clear_work_data();
void move_people_restrict(Snation *np);
void move_people_free(Snation *np);
int emp_desire(Snation *np, int a, int b);
int get_n_act_ntn(Sworld *wp);
int get_avg_civil(Sworld *wp);
int get_avg_soldiers(Sworld *wp);
int get_avg_money(Sworld *wp);
int get_avg_metal(Sworld *wp);
int get_avg_jewels(Sworld *wp);
int get_avg_food(Sworld *wp);
int get_avg_sectors(Sworld *wp);
int get_per_occu_land(Sworld *wp);
int get_per_occu_water(Sworld *wp);
void sort_ptlist(struct pt_list **ptlist);
void free_ptlist(struct pt_list **ptlist);
void add_to_plist(struct pt_list **ptlist, int x, int y);
void get_tech_entry(FILE *fp, Snation *np, FILE *mailfile);
void skip_tech_entry(FILE *fp);

#endif // _FUNCTIONS_H_
