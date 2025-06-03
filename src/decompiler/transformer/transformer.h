#ifndef GARLIC_TRANSFORMER_H
#define GARLIC_TRANSFORMER_H

#include "decompiler/structure.h"
#include "common/str_tools.h"

string exp_to_s(jd_exp *expression);

string exp_invoke_to_s(jd_exp *expression);

string exp_invokeinterface_to_s(jd_exp *expression);

string exp_invokespecial_to_s(jd_exp *expression);

string exp_invokestatic_to_s(jd_exp *expression);

string exp_invokevirtual_to_s(jd_exp *expression);

string exp_invokedynamic_to_s(jd_exp *expression);

string exp_stack_value_to_s(jd_exp *expression);

string exp_local_variable_to_s(jd_exp *expression);

string exp_const_to_s(jd_exp *expression);

string exp_if_to_s(jd_exp *expression);

string exp_get_field_to_s(jd_exp *expression);

string exp_put_field_to_s(jd_exp *expression);

string exp_get_static_to_s(jd_exp *expression);

string exp_put_static_to_s(jd_exp *expression);

string exp_return_to_s(jd_exp *expression);

string exp_array_store_to_s(jd_exp *expression);

string exp_array_load_to_s(jd_exp *expression);

string exp_new_array_to_s(jd_exp *expression);

string exp_arraylength_to_s(jd_exp *expression);

string exp_switch_to_s(jd_exp *expression);

string exp_goto_to_s(jd_exp *expression);

string exp_lvalue_to_s(jd_exp *expression);

string get_operator_name(jd_operator op);

string exp_operator_to_s(jd_exp *expression);

string exp_single_operator_to_s(jd_exp *expression);

string exp_single_list_to_s(jd_exp *expression);

string exp_instanceof_to_s(jd_exp *expression);

string exp_ternary_to_s(jd_exp *expression);

string exp_break_to_s(jd_exp *expression);

string exp_continue_to_s(jd_exp *expression);

string exp_while_to_s(jd_exp *expression);

string exp_do_while_to_s(jd_exp *expression);

string exp_for_to_s(jd_exp *expression);

string exp_logic_not_to_s(jd_exp *expression);

string exp_assignment_to_s(jd_exp *expression);

string exp_assignment_chain_to_s(jd_exp *expression);

string exp_stack_var_to_s(jd_exp *expression);

string exp_uninitialize_to_s(jd_exp *expression);

string exp_initialize_to_s(jd_exp *expression);

string exp_cast_to_s(jd_exp *expression);

string exp_store_to_s(jd_exp *expression);

string exp_define_stack_var_to_s(jd_exp *expression);

string exp_athrow_to_s(jd_exp *expression);

string exp_iinc_to_s(jd_exp *expression);

string exp_declaration_to_s(jd_exp *expression);

string exp_assert_to_s(jd_exp *expression);

string exp_lambda_to_s(jd_exp *expression);

string exp_anonymous_to_s(jd_exp *expression);

string exp_monitorenter_to_s(jd_exp *expression);

string exp_monitorexit_to_s(jd_exp *expression);

string exp_str_concat_to_s(jd_exp *expression);

string exp_enum_to_s(jd_exp *expression);

string exp_if_break_to_s(jd_exp *expression);

void expression_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_invoke_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_invokeinterface_to_stream(FILE *stream,
                                   jd_node *node,
                                   jd_exp *expression);

void exp_invokespecial_to_stream(FILE *stream,
                                 jd_node *node,
                                 jd_exp *expression);

void exp_invokestatic_to_stream(FILE *stream,
                                jd_node *node,
                                jd_exp *expression);

void exp_invokevirtual_to_stream(FILE *stream,
                                 jd_node *node,
                                 jd_exp *expression);

void exp_invokedynamic_to_stream(FILE *stream,
                                 jd_node *node,
                                 jd_exp *expression);

void exp_stack_value_to_stream(FILE *stream,
                               jd_node *node,
                               jd_exp *expression);

void exp_local_variable_to_stream(FILE *stream,
                                  jd_node *node,
                                  jd_exp *expression);

void exp_const_to_stream(FILE *stream,
                         jd_node *node,
                         jd_exp *expression);

void exp_if_to_stream(FILE *stream,
                      jd_node *node,
                      jd_exp *expression);

void exp_get_field_to_stream(FILE *stream,
                             jd_node *node,
                             jd_exp *expression);

void exp_put_field_to_stream(FILE *stream,
                             jd_node *node,
                             jd_exp *expression);

void exp_get_static_to_stream(FILE *stream,
                              jd_node *node,
                              jd_exp *expression);

void exp_put_static_to_stream(FILE *stream,
                              jd_node *node,
                              jd_exp *expression);

void exp_return_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_array_store_to_stream(FILE *stream,
                               jd_node *node,
                               jd_exp *expression);

void exp_array_load_to_stream(FILE *stream,
                              jd_node *node,
                              jd_exp *expression);

void exp_new_array_to_stream(FILE *stream,
                             jd_node *node,
                             jd_exp *expression);

void exp_arraylength_to_stream(FILE *stream,
                               jd_node *node,
                               jd_exp *expression);

void exp_switch_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_goto_to_stream(FILE *stream,
                        jd_node *node,
                        jd_exp *expression);

void exp_lvalue_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_operator_to_stream(FILE *stream,
                            jd_node *node,
                            jd_exp *expression);

void exp_single_operator_to_stream(FILE *stream,
                                   jd_node *node,
                                   jd_exp *expression);

void exp_single_list_to_stream(FILE *stream,
                               jd_node *node,
                               jd_exp *expression);

void exp_instanceof_to_stream(FILE *stream,
                              jd_node *node,
                              jd_exp *expression);

void exp_ternary_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_break_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_continue_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_while_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_do_while_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_for_to_stream(FILE *stream,
                       jd_node *node,
                       jd_exp *expression);

void exp_logic_not_to_stream(FILE *stream,
                             jd_node *node,
                             jd_exp *expression);

void exp_assignment_to_stream(FILE *stream,
                              jd_node *node,
                              jd_exp *expression);

void exp_assignment_chain_to_stream(FILE *stream,
                                    jd_node *node,
                                    jd_exp *expression);

void exp_stack_var_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_uninitialize_to_stream(FILE *stream,
                                jd_node *node,
                                jd_exp *expression);

void exp_initialize_to_stream(FILE *stream,
                              jd_node *node,
                              jd_exp *expression);

void exp_cast_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_store_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_define_stack_var_to_stream(FILE *stream,
                                    jd_node *node,
                                    jd_exp *expression);

void exp_athrow_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_iinc_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_declaration_to_stream(FILE *stream,
                               jd_node *node,
                               jd_exp *expression);

void exp_assert_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_lambda_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_anonymous_to_stream(FILE *stream,
                          jd_node *node,
                          jd_exp *expression);

void exp_monitorenter_to_stream(FILE *stream,
                                jd_node *node,
                                jd_exp *expression);

void exp_monitorexit_to_stream(FILE *stream,
                               jd_node *node,
                               jd_exp *expression);

void exp_str_concat_to_stream(FILE *stream,
                              jd_node *node,
                              jd_exp *expression);

void exp_enum_to_stream(FILE *stream, jd_node *node,  jd_exp *expression);

void exp_if_break_to_stream(FILE *stream, jd_node *node, jd_exp *expression);
#endif //GARLIC_TRANSFORMER_H


