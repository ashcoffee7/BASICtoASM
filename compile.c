#include "compile.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int if_count = 0;
int while_count = 1;

bool check_const(node_t *node) {
    if (node->type == NUM) {
        return true; 
    }
    else if (node->type == BINARY_OP) {
        binary_node_t *b_node = (binary_node_t *) node;
        if ((b_node->op != '>') && (b_node->op != '<') && (b_node->op != '=')) {
            bool left = check_const(((binary_node_t *) node)->left);
            bool right = check_const(((binary_node_t *) node)->right);
            return left && right;
        }
    }
    else {
        return false;
    }
    return false;
}

long evaluate_const(node_t *node) {
    if (check_const(node)) {
        if (node->type == NUM) {
            return ((num_node_t *) node)->value;
        }
        else if (node->type == BINARY_OP) {
            binary_node_t *curr = (binary_node_t *) node;
            if ((curr->left->type == NUM) && (curr->right->type == NUM)) {
                long left_val = ((num_node_t *) curr->left)->value;
                long right_val = ((num_node_t *) curr->right)->value;
                if (curr->op == '*') {
                    return left_val * right_val;
                }
                else if (curr->op == '/') {
                    if (right_val != 0) {
                        return left_val / right_val;
                    }
                }
                else if (curr->op == '+') {
                    return left_val + right_val;
                }
                else if (curr->op == '-') {
                    return left_val - right_val;
                }
            }
            else if ((curr->left->type == BINARY_OP) && (curr->right->type == NUM)) {
                binary_node_t *left_val = ((binary_node_t *) curr->left);
                long right_val = ((num_node_t *) curr->right)->value;
                if (curr->op == '*') {
                    return evaluate_const((node_t *) left_val) * right_val;
                }
                else if (curr->op == '/') {
                    if (right_val != 0) {
                        return evaluate_const((node_t *) left_val) / right_val;
                    }
                }
                else if (curr->op == '+') {
                    return evaluate_const((node_t *) left_val) + right_val;
                }
                else if (curr->op == '-') {
                    return evaluate_const((node_t *) left_val) - right_val;
                }
            }
            else if ((curr->left->type == NUM) && (curr->right->type == BINARY_OP)) {
                long left_val = ((num_node_t *) curr->left)->value;
                binary_node_t *right_val = ((binary_node_t *) curr->right);
                if (curr->op == '*') {
                    return left_val * evaluate_const((node_t *) right_val);
                }
                else if (curr->op == '/') {
                    int right_const = evaluate_const((node_t *) right_val);
                    if (right_const != 0) {
                        return left_val / right_const;
                    }
                }
                else if (curr->op == '+') {
                    return left_val + evaluate_const((node_t *) right_val);
                }
                else if (curr->op == '-') {
                    return left_val - evaluate_const((node_t *) right_val);
                }
            }
            else if ((curr->left->type == BINARY_OP) &&
                     (curr->right->type == BINARY_OP)) {
                binary_node_t *left_val = ((binary_node_t *) curr->left);
                binary_node_t *right_val = ((binary_node_t *) curr->right);
                if (curr->op == '*') {
                    return evaluate_const((node_t *) left_val) *
                           evaluate_const((node_t *) right_val);
                }
                else if (curr->op == '/') {
                    if (evaluate_const((node_t *) right_val) != 0) {
                        return evaluate_const((node_t *) left_val) /
                               evaluate_const((node_t *) right_val);
                    }
                }
                else if (curr->op == '+') {
                    return evaluate_const((node_t *) left_val) +
                           evaluate_const((node_t *) right_val);
                }
                else if (curr->op == '-') {
                    return evaluate_const((node_t *) left_val) -
                           evaluate_const((node_t *) right_val);
                }
            }
            return 0;
        }
        return 0;
    }
    return 0;
}

int divide_two(value_t val) {
    long n = (long) val;
    if (n == 0) {
        return 0;
    }
    while (n != 1) {
        if (n % 2 != 0) {
            return 0;
        }
        n = n / 2;
    }
    return 1;
}

bool compile_ast(node_t *node) {
    if (check_const(node)) {
        printf("movq $%ld, %%rdi\n", evaluate_const(node));
    }
    else {
        node_type_t type = node->type;
        switch (type) {
            case (PRINT): {
                print_node_t *statement = (print_node_t *) node;
                node_t *expr = statement->expr;
                compile_ast(expr); // moves expr into rdi
                printf("%s\n", "callq print_int");
                break;
            }
            case (NUM): {
                num_node_t *num_node = (num_node_t *) node;
                value_t val = num_node->value;
                printf("movq $%ld, %%rdi\n", val);
                break;
            }
            case (SEQUENCE): {
                sequence_node_t *seq = (sequence_node_t *) node;
                size_t s_int = seq->statement_count;
                node_t **arr = seq->statements;
                for (size_t i = 0; i < s_int; i++) {
                    compile_ast(arr[i]);
                }
                break;
            }
            case (BINARY_OP): {
                binary_node_t *bin_t = (binary_node_t *) node;
                char base = bin_t->op;
                if (base == '*') {
                    compile_ast(bin_t->left);
                    if (bin_t->right->type == NUM) {
                        num_node_t *curr = (num_node_t *) (bin_t->right);
                        
                        if (divide_two(curr->value) == 1) {
                            long shift = log(curr->value) / log(2);
                            printf("shlq $%ld, %%rdi\n", shift);
                        }

                        else {
                            printf("push %%rdi\n");
                            compile_ast(bin_t->right);
                            printf("pop %%rax\n");
                            printf("imulq %%rax, %%rdi\n");
                        }
                    }

                    else {
                        printf("push %%rdi\n");
                        compile_ast(bin_t->right);
                        printf("pop %%rax\n");
                        printf("imulq %%rax, %%rdi\n");
                    }
                }

                if (base == '/') {
                    /*
                    pseudocode:
                    push %rdi
                    pop %rax
                    cmp 0x0, %rax
                    jneq next_line
                    idivq %rax, %rdi
                    */
                    compile_ast(bin_t->left);
                    printf("push %%rdi\n");
                    compile_ast(bin_t->right);
                    printf("pop %%rax\n");
                    printf("cqto \n");
                    printf("idiv %%rdi\n");
                    printf("movq %%rax, %%rdi\n");
                }
                if (base == '+') {
                    compile_ast(bin_t->left);
                    printf("push %%rdi\n");
                    compile_ast(bin_t->right);
                    printf("pop %%rax\n");
                    printf("addq %%rax, %%rdi\n");
                }
                if (base == '-') {
                    compile_ast(bin_t->right);
                    printf("push %%rdi\n");
                    compile_ast(bin_t->left);
                    printf("pop %%rax\n");
                    printf("subq %%rax, %%rdi\n");
                }
                if (base == '>' || base == '<' || base == '=') {
                    compile_ast(bin_t->right);
                    printf("push %%rdi\n");
                    compile_ast(bin_t->left);
                    printf("pop %%rax\n");
                    printf("cmp %%rax, %%rdi\n");
                }
                break;
            }
            case (VAR): {
                var_node_t *var = (var_node_t *) node;
                int32_t idx = ((int) var->name - 'A' + 1) * -8;
                printf("movq %d(%%rbp), %%rdi\n", idx);
                break;
            }
            case (LET): {
                let_node_t *let = (let_node_t *) node;
                int32_t idx = ((int) (let->var) - 'A' + 1) * -8;
                compile_ast(let->value);
                printf("movq %%rdi, %d(%%rbp)\n", idx);
                break;
            }
            case (IF): {
                size_t if_local = if_count;
                if_count++;
                if_node_t *conditional = (if_node_t *) node;
                node_t *if_cond = conditional->if_branch;
                node_t *else_cond = conditional->else_branch;
                compile_ast((node_t *) conditional->condition);
                if (conditional->condition->op == '=') {
                    printf("je IF_LABEL_%zu\n", if_local);
                }
                else if (conditional->condition->op == '>') {
                    printf("jg IF_LABEL_%zu\n", if_local);
                }
                else {
                    printf("jl IF_LABEL_%zu\n ", if_local);
                }
                if (else_cond != NULL) {
                    compile_ast(else_cond);
                }
                printf("jmp COND_LABEL_%zu\n", if_local);
                printf("IF_LABEL_%zu:\n ", if_local);
                compile_ast(if_cond);
                printf("COND_LABEL_%zu:\n ", if_local);
                break;
            }
            case (WHILE): {
                size_t while_local = while_count;
                while_count++;
                while_node_t *loop = (while_node_t *) node;
                binary_node_t *while_cond = loop->condition;
                printf("WHILE_LABEL_%zu:\n", while_local);
                compile_ast((node_t *) while_cond);
                if (while_cond->op == '=') {
                    printf("jne END_WHILE_LABEL_%zu\n", while_local);
                }
                else if (while_cond->op == '>') {
                    printf("jng END_WHILE_LABEL_%zu\n", while_local);
                }
                else {
                    printf("jnl END_WHILE_LABEL_%zu\n", while_local);
                }
                compile_ast((node_t *) loop->body);
                printf("jmp WHILE_LABEL_%zu\n", while_local);
                printf("END_WHILE_LABEL_%zu:\n", while_local);
                break;
            }
        }
    }

    return true; // for now, every statement causes a compilation failure
}
