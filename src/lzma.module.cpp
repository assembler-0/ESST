#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <memory>
#include <algorithm>
#include <cstring>
#include <zlib.h>
#include <lzma.h>

class CompressNDecompress {
private:
    std::atomic<bool> running{false};
    std::atomic<uint64_t> total_operations{0};
    std::atomic<uint64_t> total_bytes_processed{0};
    std::vector<std::thread> worker_threads;
    
    // Generate pseudo-random but compressible data
    std::vector<uint8_t> generate_mixed_data(size_t size, double entropy = 0.7) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        std::uniform_real_distribution<> entropy_dis(0.0, 1.0);
        
        for (size_t i = 0; i < size; ++i) {
            if (entropy_dis(gen) < entropy) {
                // Random data (harder to compress)
                data[i] = dis(gen);
            } else {
                // Repeated patterns (easier to compress)
                data[i] = (i % 16) * 16 + (i % 4);
            }
        }
        return data;
    }
    
    // LZMA compression (most CPU intensive)
    static bool lzma_compress_decompress(const std::vector<uint8_t>& input) {
        // Compression
        size_t compressed_size = input.size() + input.size() / 3 + 128;
        std::vector<uint8_t> compressed(compressed_size);
        
        lzma_stream strm = LZMA_STREAM_INIT;
        lzma_ret ret = lzma_easy_encoder(&strm, 9, LZMA_CHECK_CRC64); // Max compression
        if (ret != LZMA_OK) return false;
        
        strm.next_in = input.data();
        strm.avail_in = input.size();
        strm.next_out = compressed.data();
        strm.avail_out = compressed.size();
        
        ret = lzma_code(&strm, LZMA_FINISH);
        if (ret != LZMA_STREAM_END) {
            lzma_end(&strm);
            return false;
        }
        
        compressed_size = strm.total_out;
        lzma_end(&strm);
        compressed.resize(compressed_size);
        
        // Decompression
        std::vector<uint8_t> decompressed(input.size());
        lzma_stream strm2 = LZMA_STREAM_INIT;
        ret = lzma_stream_decoder(&strm2, UINT64_MAX, LZMA_CONCATENATED);
        if (ret != LZMA_OK) return false;
        
        strm2.next_in = compressed.data();
        strm2.avail_in = compressed.size();
        strm2.next_out = decompressed.data();
        strm2.avail_out = decompressed.size();
        
        ret = lzma_code(&strm2, LZMA_FINISH);
        lzma_end(&strm2);
        
        // Verify integrity
        return (ret == LZMA_STREAM_END) && (decompressed == input);
    }
    
    // DEFLATE compression (zlib)
    bool deflate_compress_decompress(const std::vector<uint8_t>& input) {
        // Compression
        uLongf compressed_size = compressBound(input.size());
        std::vector<uint8_t> compressed(compressed_size);
        
        int ret = compress2(compressed.data(), &compressed_size, 
                           input.data(), input.size(), Z_BEST_COMPRESSION);
        if (ret != Z_OK) return false;
        
        compressed.resize(compressed_size);
        
        // Decompression
        uLongf decompressed_size = input.size();
        std::vector<uint8_t> decompressed(decompressed_size);
        
        ret = uncompress(decompressed.data(), &decompressed_size,
                        compressed.data(), compressed.size());
        
        // Verify integrity
        return (ret == Z_OK) && (decompressed_size == input.size()) && 
               (std::memcmp(decompressed.data(), input.data(), input.size()) == 0);
    }
    
    // Multi-pass compression worker
    void compression_worker(const int worker_id, const size_t data_size, const int algorithm_mix) {
        std::cout << "Thread " << worker_id << " starting compression...\n";
        
        uint64_t local_ops = 0;
        uint64_t local_bytes = 0;
        
        while (running.load()) {
            // Generate different data patterns for each iteration
            auto data = generate_mixed_data(data_size, 0.3 + (local_ops % 7) * 0.1);
            
            bool success = false;
            
            // Alternate between algorithms for maximum CPU variety
            switch ((local_ops + worker_id) % algorithm_mix) {
                case 0:
                case 1:
                    success = lzma_compress_decompress(data);
                    break;
                default:
                    success = deflate_compress_decompress(data);
                    break;
            }
            
            if (success) {
                local_ops++;
                local_bytes += data_size * 2; // Compression + decompression
                
                // Add extra CPU load with verification pass
                if (local_ops % 10 == 0) {
                    // Generate and compress larger chunks periodically
                    auto big_data = generate_mixed_data(data_size * 3, 0.8);
                    lzma_compress_decompress(big_data);
                    local_bytes += big_data.size() * 2;
                }
            }
            
            // Update global counters periodically
            if (local_ops % 50 == 0) {
                total_operations.fetch_add(local_ops);
                total_bytes_processed.fetch_add(local_bytes);
                local_ops = 0;
                local_bytes = 0;
            }
        }
        
        // Final update
        total_operations.fetch_add(local_ops);
        total_bytes_processed.fetch_add(local_bytes);
        
        std::cout << "Thread " << worker_id << " finished.\n";
    }
    
public:
    void start(int duration_seconds = 60, 
                           size_t chunk_size = 1024 * 512) {
        
        if (running.load()) {
            std::cout << "Compression stress already running!\n";
            return;
        }
        
        running.store(true);
        total_operations.store(0);
        total_bytes_processed.store(0);
        
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 8;
        
        std::cout << "Starting compression stress test:\n";
        std::cout << "- Threads: " << num_threads << "\n";
        std::cout << "- Chunk size: " << chunk_size / 1024 << "KB\n";
        std::cout << "- Duration: " << duration_seconds << " seconds\n";
        std::cout << "- Algorithms: LZMA (level 9) + DEFLATE (max)\n\n";
        
        // Launch worker threads
        worker_threads.clear();
        for (unsigned int i = 0; i < num_threads; ++i) {
            worker_threads.emplace_back(&CompressNDecompress::compression_worker, 
                                      this, i, chunk_size, 3);
        }
        
        // Run for specified duration with progress updates
        auto start_time = std::chrono::steady_clock::now();
        for (int elapsed = 0; elapsed < duration_seconds; ++elapsed) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            auto ops = total_operations.load();
            auto bytes = total_bytes_processed.load();
            double mb_per_sec = (bytes / (1024.0 * 1024.0)) / (elapsed + 1);
            
            std::cout << "Progress: " << elapsed + 1 << "s | "
                      << "Operations: " << ops << " | "
                      << "Throughput: " << std::fixed << std::setprecision(1) 
                      << mb_per_sec << " MB/s\r" << std::flush;
        }
        
        std::cout << "\nStopping compression stress...\n";
        running.store(false);
        
        // Wait for all threads to finish
        for (auto& thread : worker_threads) {
            thread.join();
        }
        
        // Final statistics
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        
        std::cout << "\n=== COMPRESSION STRESS RESULTS ===\n";
        std::cout << "Total operations: " << total_operations.load() << "\n";
        std::cout << "Total data processed: " << total_bytes_processed.load() / (1024*1024) << " MB\n";
        std::cout << "Average throughput: " << (total_bytes_processed.load() / (1024.0*1024.0)) / (total_time/1000.0) << " MB/s\n";
        std::cout << "Operations per second: " << (total_operations.load() * 1000.0) / total_time << "\n";
        std::cout << "====================================\n\n";
    }
    
    void stop() {
        running.store(false);
    }
    
    bool is_running() const {
        return running.load();
    }
};

// Usage example
extern "C" void startLZMA(const int duration) {
    CompressNDecompress test;
    test.start(duration, 1024 * 512);
}
