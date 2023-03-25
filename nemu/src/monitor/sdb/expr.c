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
#include <memory/paddr.h>
enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_DEC_NUM,
  TK_HEX_NUM,
  TK_REG,
  TK_NEQ,
  TK_AND,
  TK_DEREF,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +",           TK_NOTYPE},// spaces
  {"\\+",                '+'},// plus
  {"==",               TK_EQ},// equal
  {"!=",              TK_NEQ},// not equal
  {"\\-",                '-'},// mines
  {"\\*",                '*'},// times
  {"\\/",                '/'},// divid
  {"\\(",                '('},// left
  {"\\)",                ')'},// right
  {"0x[A-Za-z0-9]{1,}", TK_HEX_NUM},// hex number 
  {"\\$[a-z0-9]{1,}[0-9]*",TK_REG},// register
  {"[0-9]{1,}",   TK_DEC_NUM},// decimal number
  {"&&",              TK_AND},// and
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
            case '(':
                tokens[nr_token++] = (struct token){'(',"\0"};
                break;
            case ')':
                tokens[nr_token++] = (struct token){')',"\0"};
                break;
            case TK_HEX_NUM:
                struct token hex_num;
                hex_num.type = TK_HEX_NUM;
                memset(hex_num.str, 0, 32);
                strncpy(hex_num.str, substr_start, substr_len);
                tokens[nr_token++] = hex_num;
                break;
            case TK_DEC_NUM:
                struct token num;
                num.type = TK_DEC_NUM;
                memset(num.str, 0, 32);
                strncpy(num.str, substr_start, substr_len);
                tokens[nr_token++] = num;
                break;
            case TK_EQ:
                tokens[nr_token++] = (struct token){TK_EQ, "\0"};
                break;
            case TK_NEQ:
                tokens[nr_token++] = (struct token){TK_NEQ, "\0"};
                break;
            case TK_AND:
                tokens[nr_token++] = (struct token){TK_AND, "\0"};
                break;
            case TK_REG:
                struct token reg;
                reg.type = TK_REG;
                memset(reg.str, 0, 32);
                strncpy(reg.str, substr_start, substr_len);
                tokens[nr_token++] = reg;
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

int find_major_op(int left, int right, bool *success)
{
    if (*success == false) return 0;
    int flag = 0;
    int and_i = -1;
    int eq_or_neq_i = -1;
    int times_or_divide_i = -1;
    int plus_or_minus_i = -1;
    for (int i = right; i >=left; i--)
    {
        //printf("i:%d,sec_i:%d,f:%d, t:%c\n",i,sec_i,flag,tokens[i].type);
        if (tokens[i].type == ')') flag++;
        if (tokens[i].type == '(') flag--;
        if (flag != 0) continue;
        if (tokens[i].type != '+'    &&
            tokens[i].type != '-'    &&
            tokens[i].type != '*'    &&
            tokens[i].type != '/'    &&
            tokens[i].type != TK_EQ  &&
            tokens[i].type != TK_NEQ &&
            tokens[i].type != TK_AND &&
            tokens[i].type != TK_DEREF) 
            continue;
        
        //get the major from the lowest op
        if (tokens[i].type == TK_DEREF) return i;
        if (tokens[i].type == TK_AND && and_i == -1) 
        {
            and_i = i;
            continue;
        }
        
        if((tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ) && eq_or_neq_i == -1)
        {
            eq_or_neq_i = i;
            continue;
        }

        if((tokens[i].type == '*' || tokens[i].type == '/') && times_or_divide_i == -1)
        {
            times_or_divide_i = i;
            continue;
        }

        if((tokens[i].type == '+' || tokens[i].type == '-') && plus_or_minus_i == -1)
        {
            plus_or_minus_i = i;
        }
        //printf("i:%d,sec_i:%d,f:%d\n",i,sec_i,flag);
    }
    if (eq_or_neq_i != -1) return eq_or_neq_i;
    if (times_or_divide_i != -1) return times_or_divide_i;
    if (plus_or_minus_i != -1) return plus_or_minus_i;
    
    //printf("ret:%d\n", sec_i);
    *success = false;
    return -1;
}

long long eval(int left, int right, bool *success)
{
    if (*success == false) return 0;
    
    if (left > right) return 0;
    
    if (left == right) 
    {
        switch(tokens[left].type)
        {
            case TK_DEC_NUM: return atoi(tokens[left].str);
            case TK_HEX_NUM: return strtol(tokens[left].str, NULL, 0);
            case TK_REG    : return isa_reg_str2val(tokens[left].str, success);
            default:
                 *success = false;
                 printf("Exception: No right Type!\n");
                 return 0;
        }
    }
    
    if (check_parentheses(left, right)) return eval(left + 1, right -1, success);
    
    int major_op_index = find_major_op(left, right, success);
    //printf("major_op: %d\n", major_op_index);
    if (*success == false) {
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
        case TK_EQ:  return val1==val2;
        case TK_NEQ: return val1!=val2;
        case TK_AND: return val1&&val2;
        case TK_DEREF: return paddr_read(val2, 4);
        default:
            assert(0);
    }

}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */

  for (int i = 0; i < nr_token; i++)
  {
      if (tokens[i].type == '*' &&
          (i == 0                    || 
           tokens[i-1].type == '+'   ||
           tokens[i-1].type == '-'   ||
           tokens[i-1].type == '*'   ||
           tokens[i-1].type == '/'   ||
           tokens[i-1].type == TK_EQ ||
           tokens[i-1].type == TK_NEQ ||
           tokens[i-1].type == TK_AND))
      {
          tokens[i].type = TK_DEREF;
      }
  }

  *success = true;
  word_t ret = eval(0, nr_token-1, success);
  return ret;

}
