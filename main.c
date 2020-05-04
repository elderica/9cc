/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    setlocale(LC_CTYPE, "C");  // isalnum(3)に正しく判定させる
    user_input = argv[1];
    token = tokenize(user_input);
    program();
    gencode();

   return 0;
}
