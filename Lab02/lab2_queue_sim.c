#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* =========================================================================
 * NETWORK PARAMETERS
 * ========================================================================= */
#define PACKET_LENGTH_BYTES     1000          // L = 1000 Bytes (8000 bits)
#define LINK1_BANDWIDTH_BPS     10000000.0    // R1 = 10 Mbps (Source -> Router)
#define LINK2_BANDWIDTH_BPS     10000000.0    // R2 = 10 Mbps (Router -> Destination)
#define LINK1_PROP_DELAY_SEC    0.001         // d_prop1 = 1 ms
#define LINK2_PROP_DELAY_SEC    0.002         // d_prop2 = 2 ms
#define ROUTER_PROC_DELAY_SEC   0.0001        // d_proc = 0.1 ms
#define QUEUE_CAPACITY          20            // Router buffer limit
#define TOTAL_PACKETS           100000        // Number of packets per experiment
#define RANDOM_SEED             42            // Deterministic seed for reproducibility

// Generate exponentially distributed inter-arrival times
double generate_exponential(double rate) {
    double u = (double)rand() / ((double)RAND_MAX + 1.0);
    while (u <= 0.0) {
        u = (double)rand() / ((double)RAND_MAX + 1.0);
    }
    return -log(1.0 - u) / rate;
}

// Structure to hold experiment results
typedef struct {
    double rho;
    double lambda_agg;
    long packets_generated;
    long packets_delivered;
    long packets_dropped;
    double drop_probability;
    double avg_queueing_delay;
    double avg_e2e_delay;
    int max_queue_occupancy;
} SimResult;

SimResult run_multi_source_simulation(int num_sources, double *rates, double rho) {
    SimResult res;
    res.rho = rho;
    res.packets_generated = TOTAL_PACKETS;
    res.packets_delivered = 0;
    res.packets_dropped = 0;
    res.max_queue_occupancy = 0;

    double total_lambda = 0.0;
    for (int s = 0; s < num_sources; s++) {
        total_lambda += rates[s];
    }
    res.lambda_agg = total_lambda;

    double total_queueing_delay = 0.0;
    double total_e2e_delay = 0.0;

    double packet_bits = (double)PACKET_LENGTH_BYTES * 8.0;
    double d_trans1 = packet_bits / LINK1_BANDWIDTH_BPS;
    double d_trans2 = packet_bits / LINK2_BANDWIDTH_BPS;

    // Track next arrival timestamp per source
    double *next_arr_times = (double *)malloc(sizeof(double) * num_sources);
    for (int s = 0; s < num_sources; s++) {
        next_arr_times[s] = generate_exponential(rates[s]);
    }

    // Circular queue to track start transmission times of buffered packets
    double *queue_start_trans = (double *)malloc(sizeof(double) * QUEUE_CAPACITY);
    int q_head = 0, q_tail = 0, q_count = 0;

    double last_link2_free_time = 0.0;

    for (long i = 0; i < TOTAL_PACKETS; i++) {
        // Find source with the earliest packet ready
        int earliest_src = 0;
        double min_gen_time = next_arr_times[0];
        for (int s = 1; s < num_sources; s++) {
            if (next_arr_times[s] < min_gen_time) {
                min_gen_time = next_arr_times[s];
                earliest_src = s;
            }
        }

        double current_gen_time = min_gen_time;
        // Advance this source's next packet generation time
        next_arr_times[earliest_src] += generate_exponential(rates[earliest_src]);

        // Link 1 & router ingress
        double router_arr_time = current_gen_time + d_trans1 + LINK1_PROP_DELAY_SEC;
        double ready_time = router_arr_time + ROUTER_PROC_DELAY_SEC;

        // Drain queue of completed waiting packets
        while (q_count > 0 && queue_start_trans[q_head] <= router_arr_time) {
            q_head = (q_head + 1) % QUEUE_CAPACITY;
            q_count--;
        }

        // Bounded FIFO Queue Check
        if (q_count >= QUEUE_CAPACITY) {
            res.packets_dropped++;
        } else {
            res.packets_delivered++;

            double trans2_start = (last_link2_free_time > ready_time) ? last_link2_free_time : ready_time;
            double q_delay = trans2_start - ready_time;
            if (q_delay < 0.0) q_delay = 0.0;

            double trans2_end = trans2_start + d_trans2;
            last_link2_free_time = trans2_end;

            double dest_arr = trans2_end + LINK2_PROP_DELAY_SEC;
            double e2e = dest_arr - current_gen_time;

            total_queueing_delay += q_delay;
            total_e2e_delay += e2e;

            queue_start_trans[q_tail] = trans2_start;
            q_tail = (q_tail + 1) % QUEUE_CAPACITY;
            q_count++;

            if (q_count > res.max_queue_occupancy) {
                res.max_queue_occupancy = q_count;
            }
        }
    }

    free(next_arr_times);
    free(queue_start_trans);

    res.drop_probability = (double)res.packets_dropped / (double)TOTAL_PACKETS;
    res.avg_queueing_delay = (res.packets_delivered > 0) ? (total_queueing_delay / res.packets_delivered) : 0.0;
    res.avg_e2e_delay = (res.packets_delivered > 0) ? (total_e2e_delay / res.packets_delivered) : 0.0;

    return res;
}

int main() {
    srand(RANDOM_SEED);
    double packet_bits = (double)PACKET_LENGTH_BYTES * 8.0;

    /* =========================================================================
     * PART 1: 4 Sources with Equal Rates across 7 Traffic Intensities
     * ========================================================================= */
    printf("\n====================================================================================================\n");
    printf("                                   PART 1: EQUAL RATE SOURCES\n");
    printf("====================================================================================================\n");
    printf("%-6s | %-12s | %-10s | %-10s | %-10s | %-10s | %-14s | %-14s | %-6s\n",
           "rho", "lambda_agg", "Generated", "Delivered", "Dropped", "P_Drop", "Avg Q_Delay(s)", "Avg E2E(s)", "Max_Q");
    printf("----------------------------------------------------------------------------------------------------\n");

    FILE *fp1 = fopen("lab2_part1_results.csv", "w");
    if (fp1) {
        fprintf(fp1, "Traffic Intensity (rho),Aggregate Packet Rate (lambda),Packets Generated,Packets Delivered,Packets Dropped,Packet-Drop Probability,Avg Queueing Delay (s),Avg End-to-End Delay (s),Max Queue Occupancy\n");
    }

    double rho_values[] = {0.2, 0.4, 0.6, 0.8, 0.9, 1.0, 1.2};
    int n_rho = sizeof(rho_values) / sizeof(rho_values[0]);

    for (int i = 0; i < n_rho; i++) {
        double rho = rho_values[i];
        double total_lambda = (rho * LINK2_BANDWIDTH_BPS) / packet_bits;
        double rates[4] = {total_lambda / 4.0, total_lambda / 4.0, total_lambda / 4.0, total_lambda / 4.0};

        SimResult res = run_multi_source_simulation(4, rates, rho);

        if (fp1) {
            fprintf(fp1, "%.2f,%.2f,%ld,%ld,%ld,%.6f,%.6f,%.6f,%d\n",
                    res.rho, res.lambda_agg, res.packets_generated, res.packets_delivered,
                    res.packets_dropped, res.drop_probability, res.avg_queueing_delay,
                    res.avg_e2e_delay, res.max_queue_occupancy);
        }

        printf("%-6.2f | %-12.2f | %-10ld | %-10ld | %-10ld | %-10.4f | %-14.6f | %-14.6f | %-6d\n",
               res.rho, res.lambda_agg, res.packets_generated, res.packets_delivered,
               res.packets_dropped, res.drop_probability, res.avg_queueing_delay,
               res.avg_e2e_delay, res.max_queue_occupancy);
    }
    if (fp1) fclose(fp1);

    /* =========================================================================
     * PART 2: 4 Sources with Heterogeneous Rate Distributions (Constant Aggregate)
     * ========================================================================= */
    printf("\n====================================================================================================\n");
    printf("                             PART 2: HETEROGENEOUS RATE CONFIGURATIONS\n");
    printf("====================================================================================================\n");
    printf("%-24s | %-6s | %-10s | %-10s | %-10s | %-10s | %-14s | %-14s\n",
           "Distribution (l1,l2,l3,l4)", "rho", "Generated", "Delivered", "Dropped", "P_Drop", "Avg Q_Delay(s)", "Avg E2E(s)");
    printf("----------------------------------------------------------------------------------------------------\n");

    FILE *fp2 = fopen("lab2_part2_results.csv", "w");
    if (fp2) {
        fprintf(fp2, "Distribution,Traffic Intensity (rho),Aggregate Packet Rate (lambda),Packets Generated,Packets Delivered,Packets Dropped,Packet-Drop Probability,Avg Queueing Delay (s),Avg End-to-End Delay (s),Max Queue Occupancy\n");
    }

    double part2_rates[4][4] = {
        {100.0, 100.0, 100.0, 700.0},
        {150.0, 150.0, 250.0, 450.0},
        {250.0, 250.0, 250.0, 250.0},
        {400.0, 300.0, 200.0, 100.0}
    };
    const char *dist_labels[] = {
        "(100, 100, 100, 700)",
        "(150, 150, 250, 450)",
        "(250, 250, 250, 250)",
        "(400, 300, 200, 100)"
    };

    for (int i = 0; i < 4; i++) {
        double agg_lambda = part2_rates[i][0] + part2_rates[i][1] + part2_rates[i][2] + part2_rates[i][3];
        double rho = (agg_lambda * packet_bits) / LINK2_BANDWIDTH_BPS; // = (1000 * 8000) / 10,000,000 = 0.80

        SimResult res = run_multi_source_simulation(4, part2_rates[i], rho);

        if (fp2) {
            fprintf(fp2, "\"%s\",%.2f,%.2f,%ld,%ld,%ld,%.6f,%.6f,%.6f,%d\n",
                    dist_labels[i], res.rho, res.lambda_agg, res.packets_generated,
                    res.packets_delivered, res.packets_dropped, res.drop_probability,
                    res.avg_queueing_delay, res.avg_e2e_delay, res.max_queue_occupancy);
        }

        printf("%-24s | %-6.2f | %-10ld | %-10ld | %-10ld | %-10.4f | %-14.6f | %-14.6f\n",
               dist_labels[i], res.rho, res.packets_generated, res.packets_delivered,
               res.packets_dropped, res.drop_probability, res.avg_queueing_delay,
               res.avg_e2e_delay);
    }
    if (fp2) fclose(fp2);

    printf("====================================================================================================\n");
    printf("Simulations completed! Created 'lab2_part1_results.csv' and 'lab2_part2_results.csv'.\n\n");

    return 0;
}
