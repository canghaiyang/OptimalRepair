#ifndef BANDWIDTH_H
#define BANDWIDTH_H

int read_updown_bandwidth(int *uplink_bandwidth, int *downlink_bandwidth, int light_heavy_flag, int band_location);
void get_bandwidth_between_nodes(int *uplink_bandwidth, int *downlink_bandwidth, int **nodes_bandwidth);

#endif