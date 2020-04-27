/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];
    // トークナイズする
    token = tokenize(user_input);
    program();

    // プロローグを出力する
    // 変数のための領域を確保する
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n"); // a〜zの26個の変数×8バイト

    for (int i = 0; code[i] != NULL; i++) {
        printf("  # %s:%d i:%d\n", __FILE__, __LINE__, i);
        gen(code[i]);
        // 各式の評価結果がraxに残る(gen関数末尾参照)。
        // スタック溢れを防ぐため、popする
        printf("  pop rax\n");
    }

    // エピローグ
    // 最後の式の結果がraxに残り、返り値となる。
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
