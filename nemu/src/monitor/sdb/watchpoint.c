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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char str_expr[32];
  word_t val;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
static WP* new_wp()
{
    if (free_== NULL)
    {
        printf("no space!\n");
        assert(0);
    }
    WP* ret = head;
    if (head == NULL)
    {
        head = free_;
        head->next = NULL;
        ret=head;
    }
    else
    {
        head->next = free_;
        ret = head->next;
        ret->next=NULL;
    }

    free_ = free_->next;

    return ret;
}

static void free_wp(WP *wp)
{
    if (wp == NULL) return;
    WP* exist = head;
    if (exist->NO == wp->NO)
    {
        exist->next = free_;
        free_ = exist;
        head = NULL;
        return;
    }
    while (exist->next!=NULL)
    {
        if (wp->NO == exist->next->NO)
        {
            exist->next = exist->next->next;
            wp->next = free_;
            free_ = wp;
            return;
        }
        exist = exist->next;
    }
}

int add_watchpoint(char* str_expr, word_t val)
{
    WP* wp = new_wp();
    strncpy(wp->str_expr, str_expr, 32);
    wp->val = val;
    return wp->NO;
}

bool delete_watchpoint(int NO)
{
    WP* wp = head;
    while(wp != NULL)
    {
        if (wp->NO == NO)
        {
            free_wp(wp);
            return true;
        }
        wp = wp->next;
    }
    return false;
}

bool watchpoint_difftest()
{
    WP* cur = head;
    bool success = false;
    while (cur!=NULL)
    {
        word_t val = expr(cur->str_expr, &success);
        if (cur->val != val) 
        {
            cur->val = val;
            printf("Stop here!\n");
            return true;
        }

        cur = cur->next;
    }

    return false;
}
