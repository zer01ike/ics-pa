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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 256] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  long long int_result = %s;"
"  unsigned result = int_result; "
"  if (int_result >= 0)"
"  printf(\"%%u\", result); "
"  return 0; "
"}";
static int buf_index = 0;
static void gen(char ch)
{
    if (buf_index >=65535) return;
    buf[buf_index++] = ch;
    return;
}
static void gen_rand_num()
{
    gen(rand()%9 + '1');
    return;
}

static void gen_rand_op()
{
    int index = rand()%4;
    char op = ' ';
    switch(index)
    {
        case 1:
           op = '+';
           break;
        case 2:
           op = '-';
           break;
        case 3:
           op = '*';
           break;
        default:
           op = '/';
           break;
    }
    gen(op);
    return;
}
static void gen_rand_expr() {
  if (buf_index >= 65530) return;
  switch(rand()%3)
  {
      case 0: gen_rand_num();break;
      case 1: gen('('); gen_rand_expr();gen(')');break;
      default: gen_rand_expr();gen_rand_op();gen_rand_expr();break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf_index = 0;
    gen_rand_expr();
    gen('\0');

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);
    //if (ret > 0 && )
        //printf("%d, %u, %s\n",ret, result, buf);
    if (ret > 0)
        printf("%u %s\n", result, buf);
  }
  return 0;
}
