/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

static void gen_lval(Node *node);
static void gen(Node *node);

static unsigned int labelnumber = 0;

static void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }

    printf("  # gen_lval\n");
    printf("  lea rax, [rbp-%d]\n", node->var->offset);
    printf("  push rax\n");
}

void gen(Node *node) {
    int ln;
    switch (node->kind) {
        case ND_NUM:
            printf("  # ND_NUM\n");
            printf("  push %d\n", node->val);
            return;
        case ND_LVAR:
            printf("  # ND_LVAR\n");
            gen_lval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
        case ND_ASSIGN:
            printf("  # ND_ASSIGN\n");
            gen_lval(node->lhs);
            gen(node->rhs);

            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            return;
        case ND_EXPR_STMT:
            printf("  # ND_EXPR_STMT\n");
            gen(node->lhs);
            printf("  add rsp, 8\n");
            return;
        case ND_IF:
            ln = labelnumber++;
            if (node->els == NULL) {
                printf("  # ND_IF(els==NULL)\n");
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je .L.endif.%d\n", ln);
                gen(node->then);
                printf(".L.endif.%d:\n", ln);
            } else {
                printf("  # ND_IF(els!=NULL)\n");
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je .L.else.%d\n", ln);
                gen(node->then);
                printf("  jmp .L.endif.%d\n", ln);
                printf(".L.else.%d:\n", ln);
                gen(node->els);
                printf(".L.endif.%d:\n", ln);
            }
            return;
        case ND_WHILE:
            ln = labelnumber++;
            printf(".L.while.%d:\n", ln);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .L.endwhile.%d\n", ln);
            gen(node->body);
            printf("  jmp .L.while.%d\n", ln);
            printf(".L.endwhile.%d:\n", ln);
            return;
        case ND_FOR:
            ln = labelnumber++;
            if (node->init != NULL) {
                gen(node->init);
            }
            printf(".L.for.%d:\n", ln);
            if (node->cond != NULL) {
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je .L.endfor.%d\n", ln);
            }
            gen(node->body);
            if (node->inc != NULL) {
                gen(node->inc);
            }
            printf("  jmp .L.for.%d\n", ln);
            printf(".L.endfor.%d:\n", ln);
            return;
        case ND_RETURN:
            gen(node->lhs);
            printf("  # ND_RETURN\n");
            printf("  pop rax\n");
            printf("  jmp .L.return\n");
            return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  # %s:%d\n", __FILE__, __LINE__);
    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  # ND_ADD\n");
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  # ND_SUB\n");
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  # ND_MUL\n");
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  # ND_NIV\n");
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  # ND_EQ\n");
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  # ND_NE\n");
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  # ND_LT\n");
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  # ND_LE\n");
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }

    printf("  push rax\n");
}

// コード生成器のエントリポイント
void gencode(Function *func) {
    // 変数にオフセットを割り当てる
    int o = 0;
    for (LVar *var = func->locals; var != NULL; var = var->next) {
        o += 8;
        var->offset = o;
    }
    func->stack_size = o;

    // プロローグを出力する
    // 変数のための領域を確保する
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", func->stack_size);

    int l = 1;
    for (Node *cur = func->nodes; cur != NULL; cur = cur->next) {
        printf("  # %s:%d line:%d\n", __FILE__, __LINE__, l++);
        gen(cur);
    }

    // エピローグ
    printf(".L.return:\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}
