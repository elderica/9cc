/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

// エラーを報告するための関数
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 文字列の先頭がprefixと一致するか調べる
bool startswith(char *prefix, char *str) {
    return memcmp(prefix, str, strlen(prefix)) == 0;
}

