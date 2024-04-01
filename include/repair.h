#ifndef REPAIR_H
#define REPAIR_H

int repair_data_chunks(int *ok_node_list, int nok, int *err_node_list, int nerrs, char **data, char **coding, char **repair_data);
int encode_data_chunks(char **data, char **coding);

#endif