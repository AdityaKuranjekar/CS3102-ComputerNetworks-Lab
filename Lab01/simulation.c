#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Function to generate exponentially distributed random values for Poisson arrivals
double get_exponential_interarrival(double lambda) {
    double u = (double)rand() / RAND_MAX;
    while (u == 0 || u == 1) { // Avoid log(0) or log(1) edge cases
        u = (double)rand() / RAND_MAX;
    }
    return -log(1.0 - u) / lambda;
}

void run_experiment(double rho, double packet_len_bytes, double R_sr, double R_rd,
                    double prop_sr, double prop_rd, double proc_delay,
                    int queue_capacity, int total_packets, FILE *csv_file) {
    
    double packet_len_bits = packet_len_bytes * 8.0;
    
    // Calculate lambda based on rho: rho = (lambda * L) / R_rd  =>  lambda = (rho * R_rd) / L
    double lambda = (rho * R_rd) / packet_len_bits;
    
    // Transmission delays for both links
    double trans_sr = packet_len_bits / R_sr;
    double trans_rd = packet_len_bits / R_rd;
    
    int packets_generated = total_packets;
    int packets_delivered = 0;
    int packets_dropped = 0;
    
    double total_queueing_delay = 0.0;
    double total_end_to_end_delay = 0.0;
    
    int current_queue_size = 0;
    int max_queue_occupancy = 0;
    
    double current_time = 0.0;
    
    // Dynamic array to store departure times of packets currently in service/queue
    double *departure_times = (double *)malloc(queue_capacity * sizeof(double));
    
    for (int i = 0; i < total_packets; i++) {
        // Step A: Calculate packet generation time
        double inter_arrival = get_exponential_interarrival(lambda);
        current_time += inter_arrival;
        double generation_time = current_time;
        
        // Step B: Calculate arrival time at router
        double router_arrival_time = generation_time + trans_sr + prop_sr;
        
        // Step C: Update queue state (remove packets that left before current packet arrived)
        int updated_queue_size = 0;
        for (int k = 0; k < current_queue_size; k++) {
            if (departure_times[k] > router_arrival_time) {
                departure_times[updated_queue_size++] = departure_times[k];
            }
        }
        current_queue_size = updated_queue_size;
        
        // Step D: Process queue admission
        if (current_queue_size >= queue_capacity) {
            packets_dropped++;
        } else {
            // Packet accepted into queue
            packets_delivered++;
            
            // Determine when this packet can start service at router
            double service_start_time = router_arrival_time;
            if (current_queue_size > 0) {
                // If queue is not empty, service starts after the last queued packet completes transmission
                double last_packet_departure = departure_times[current_queue_size - 1];
                if (last_packet_departure > service_start_time) {
                    service_start_time = last_packet_departure;
                }
            }
            
            double queueing_delay = service_start_time - router_arrival_time;
            
            // Packet departs router after processing and transmission over router-to-destination link
            double router_departure_time = service_start_time + proc_delay + trans_rd;
            double dest_arrival_time = router_departure_time + prop_rd;
            
            double end_to_end_delay = dest_arrival_time - generation_time;
            
            // Accumulate metrics
            total_queueing_delay += queueing_delay;
            total_end_to_end_delay += end_to_end_delay;
            
            // Add packet departure time to the queue tracking array
            departure_times[current_queue_size] = router_departure_time;
            current_queue_size++;
            
            if (current_queue_size > max_queue_occupancy) {
                max_queue_occupancy = current_queue_size;
            }
        }
    }
    
    free(departure_times);
    
    double drop_probability = (double)packets_dropped / packets_generated;
    double avg_queueing_delay = (packets_delivered > 0) ? (total_queueing_delay / packets_delivered) : 0.0;
    double avg_end_to_end_delay = (packets_delivered > 0) ? (total_end_to_end_delay / packets_delivered) : 0.0;
    
    // Write results to CSV
    fprintf(csv_file, "%.2f,%.4f,%d,%d,%d,%.6f,%.6f,%.6f,%d\n",
            rho, lambda, packets_generated, packets_delivered, packets_dropped,
            drop_probability, avg_queueing_delay, avg_end_to_end_delay, max_queue_occupancy);
}


int main() {
    // Input parameters
    double packet_len_bytes = 1000.0;      // 1000 Bytes = 8000 bits
    double R_sr = 10000000.0;               // 10 Mbps (Source-to-Router)
    double R_rd = 1000000.0;                // 1 Mbps bottleneck link (Router-to-Dest)
    double prop_sr = 0.001;                 // 1 ms
    double prop_rd = 0.002;                 // 2 ms
    double proc_delay = 0.0001;             // 0.1 ms
    int queue_capacity = 20;               // Buffer size in packets
    int total_packets = 100000;            // Number of simulated packets
    unsigned int seed = 42;                 // Seed for reproducibility
    
    srand(seed);
    
    FILE *csv_file = fopen("results.csv", "w");
    if (!csv_file) {
        printf("Error opening output CSV file!\n");
        return 1;
    }
    
    // Write CSV Header
    fprintf(csv_file, "Traffic Intensity (rho),Lambda (pkts/sec),Generated,Delivered,Dropped,Drop Probability,Avg Queueing Delay (s),Avg End-to-End Delay (s),Max Queue Occupancy\n");
    
    // Target traffic intensity values
    double rhos[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.95, 1.0, 1.1, 1.2};
    int num_experiments = sizeof(rhos) / sizeof(rhos[0]);
    
    printf("Running simulation across %d traffic intensity levels...\n", num_experiments);
    
    for (int i = 0; i < num_experiments; i++) {
        run_experiment(rhos[i], packet_len_bytes, R_sr, R_rd, prop_sr, prop_rd,
                       proc_delay, queue_capacity, total_packets, csv_file);
    }
    
    fclose(csv_file);
    printf("Simulation completed successfully! Output saved to 'results.csv'.\n");
    
    return 0;
}
