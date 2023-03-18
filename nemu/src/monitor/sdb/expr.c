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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NUM,

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},         // mines
  {"\\*", '*'},         // times
  {"\\/", '/'},         // divid
  {"[0-9]",TK_NUM}, // number
  {"\\(", '('},         // left
  {"\\)", ')'},         // right
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[65535] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            //i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        switch (rules[i].token_type) {
            case '+':
                tokens[nr_token++] = (struct token){'+',"\0"};
                break;
            case '-':
                tokens[nr_token++] = (struct token){'-',"\0"};
                break;
            case '*':
                tokens[nr_token++] = (struct token){'*',"\0"};
                break;
            case '/':
                tokens[nr_token++] = (struct token){'/',"\0"};
                break;
            case TK_NUM:
                struct token num;
                num.type = TK_NUM;
                memset(num.str, 0, 32);
                strncpy(num.str, substr_start, substr_len);
                tokens[nr_token++] = num;
                break;
            case '(':
                tokens[nr_token++] = (struct token){'(',"\0"};
                break;
            case ')':
                tokens[nr_token++] = (struct token){')',"\0"};
                break;
            default: 
                break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int left, int right)
{
    if(tokens[left].type != '(' || tokens[right].type != ')') return false;
    int flag = 0;
    for(int i = left; i < right; i++)
    {
        if (tokens[i].type == '(') flag++;
        else if (tokens[i].type == ')') flag--;
        if (flag <= 0) return false; 
    }
    return true;
}

int find_major_op(int left, int right)
{
    int flag = 0;
    int sec_i = -1;
    for (int i = right; i >=left; i--)
    {
        //printf("i:%d,sec_i:%d,f:%d, t:%c\n",i,sec_i,flag,tokens[i].type);
        if (tokens[i].type == ')') flag++;
        else if(tokens[i].type == '(') flag--;
        else if (tokens[i].type != '+' &&
                 tokens[i].type != '-' &&
                 tokens[i].type != '*' &&
                 tokens[i].type != '/')
            continue;
        else if((tokens[i].type == '+' || tokens[i].type == '-') && flag == 0)
        {
            //printf("ret:%d\n", i);    
            return i;
        }

        else if((tokens[i].type == '*' || tokens[i].type == '/') && sec_i == -1 && flag == 0)
        {
            sec_i = i;
        }
        //printf("i:%d,sec_i:%d,f:%d\n",i,sec_i,flag);
    }
    if (sec_i == -1) return -1;
    //printf("ret:%d\n", sec_i);
    return sec_i;
}

long long eval(int left, int right, bool *success)
{
    if (*success == false) return 0;
    if (left > right) return 0;
    else if (left == right) {
        return atoi(tokens[left].str);
    }
    else if (check_parentheses(left, right)) return eval(left + 1, right -1, success);
    else
    {
        int major_op_index = find_major_op(left, right);
        //printf("major_op: %d\n", major_op_index);
        if (major_op_index < 0) {
            *success = false;
            return 0;
        }
        long long val1 = eval(left, major_op_index-1, success);
        long long val2 = eval(major_op_index+1,right, success);
        //printf("l:%lld, r:%lld, i:%d\n",val1, val2, major_op_index);
        
        switch(tokens[major_op_index].type)
        {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/': 
            {
                if (val2 == 0)
                {
                    printf("divided 0!\n");
                    *success = false;
                    return 0;
                }
                return val1 / val2;
            }
            default:
                      assert(0);
        }
    }

}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  word_t ret = eval(0, nr_token-1, success);
  return ret;

}
