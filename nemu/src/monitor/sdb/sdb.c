/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static int cmd_test_expr(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "step instruction", cmd_si },
  { "info", "info register or watchpiont info", cmd_info },
  { "x", "get the answer of expr & result will be addr", cmd_x },
  { "p", "get the answer of expr", cmd_p },
  { "w", "watch point", cmd_w },
  { "d", "delet the watch point", cmd_d },
  { "test_expr","test the expr program", cmd_test_expr},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args)
{
	char *arg = strtok(NULL, " ");
	int step = 0;
	if (arg == NULL) {
		step = 1;
	}
	else {
		step = atoi(arg);
	}
	cpu_exec(step);
	return 0;
}

static int cmd_info(char *args)
{
	char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
        return 0;
    }

    if (arg[0] == 'r')
    {
        isa_reg_display();
        return 0;
    }

    if (arg[0] == 'w')
    {
        return 0;
    }
	return 0;
}

static int cmd_x(char *args)
{
    bool success = false;
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
        return 0;
    }

    int sz = atoi(arg);
    if (sz < 0) return 0;

    arg = strtok(NULL, " ");
    if (arg == NULL)
    {
        return 0;
    }

    word_t addr = expr(arg, &success);
    for (int i = 0; i < sz; i++)
    {
        word_t content = paddr_read(addr, 4);
        printf("0x%-8lx: 0x%08lx\n", addr, content);
        addr+=sizeof(uint8_t)*4;
    }
    return 0;
}


static int cmd_p(char *args)
{
    char * arg = strtok(NULL, "\n");
    if (arg == NULL) return 0;
    bool success = false;
    word_t result = expr(arg, &success);   
    if (success)
        printf("%ld\n", result);
    else
    {
        printf("Bad Expr!\n");
    }
	return 0;
}


static int cmd_w(char *args)
{
    char* arg = strtok(NULL, "\n");
    if (arg == NULL) return 0;
    bool success = false;
    word_t val = expr(arg, &success);
    if (success)
    {
        printf("try add wp\n");
        int NO = add_watchpoint(arg, val);
        printf("add wp: %d\n", NO);
    }
    else
    {
        printf("add wp fail!\n");
    }
	return 0;
}

static int cmd_d(char *args)
{
    char * arg = strtok(NULL, "\n");
    if (arg == NULL) return 0;
    int NO = atoi(arg);
    if (delete_watchpoint(NO))
    {
        printf("delete wp Success!\n");
    }
    else
    {
        printf("delete wp Fail No such wp!\n");
    }
	return 0;
}

static int cmd_test_expr(char *args)
{
    FILE *fp = fopen("/home/kane/ics2022/nemu/tools/gen-expr/build/input","r");
    if (fp == NULL) return 0;

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    static int test_count = 0;
    static int success_count = 0;
    while ((read = getline(&line, &len, fp))!= -1)
    {
        char * arg = strtok(line, " ");
        if (arg == NULL) continue;
        test_count++;
        long unsigned int result = atoi(arg);
        arg = strtok(NULL, "\n");
        bool success = false;
        word_t expr_result = expr(arg, &success);
        if (success && expr_result == result)
        {
            success_count++;
        }
        else
        {
            printf("case: %d, result: %lu, expr_result: %lu, expr: %s\n",test_count, result, expr_result,arg);
        }
        //break;
    }
    fclose(fp);
    printf("Success Rate: %.2f%%\n", success_count*1.0f/test_count * 100);
    return 0;    
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
