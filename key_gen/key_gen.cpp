#include <array>
#include <cstdint>
#include <stdexcept>
#include <fstream>
#include <optional>
#include <filesystem>
#include <iostream>
#include <thread>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

inline void fill_random_bytes(uint8_t* dst, size_t len) {
    if (BCryptGenRandom(nullptr, dst, static_cast<ULONG>(len),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        throw std::runtime_error("BCryptGenRandom failed");
    }
}
#else
#include <sys/random.h>

inline void fill_random_bytes(uint8_t* dst, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        ssize_t ret = getrandom(dst + offset, len - offset, 0);
        if (ret < 0) {
            throw std::runtime_error("getrandom fallita");
        }
        offset += static_cast<size_t>(ret);
    }
}
#endif

std::string CONFIG_FILE{"env.txt"};

bool random_bit() {
    constexpr size_t BUF_BYTES = 64;
    thread_local std::array<uint8_t, BUF_BYTES> buf{};
    thread_local size_t byte_idx = BUF_BYTES;
    thread_local uint8_t current_byte = 0;
    thread_local int bits_left = 0;

    if (bits_left == 0) {
        if (byte_idx == BUF_BYTES) {
            fill_random_bytes(buf.data(), BUF_BYTES);
            byte_idx = 0;
        }
        current_byte = buf[byte_idx++];
        bits_left = 8;
    }

    bool bit = current_byte & 1;
    current_byte >>= 1;
    --bits_left;
    return bit;
}

class LongKeyChunk {
    private:
        long long bytesLeft{};
        long long chunkSize{};
    public:
        LongKeyChunk() = default;
        LongKeyChunk(long long nBytes, long long cS) : bytesLeft{nBytes}, chunkSize{cS} {}

        std::optional<std::vector<unsigned char>> get() {
            std::vector<unsigned char> foundChunk{};
            foundChunk.reserve(chunkSize);

            if (bytesLeft > 0) {
                auto toCheck = chunkSize <= bytesLeft ? chunkSize : bytesLeft;
                for (long long i = 0; i < toCheck; ++i) {
                    unsigned char byte{};

                    for (int j = 0; j < 8; ++j) {
                        byte = (byte << 1) | random_bit();
                    }

                --bytesLeft;
                foundChunk.push_back(byte);
                }

                return foundChunk;
        }
        return std::nullopt;
        }
};

constexpr long long pow(int b, int e) {
    long long result{1};

    for (int i = 0; i < e; ++i) {
        result *= b;
    }

    return result;
} 

long long getFileByteSize() {
    std::ifstream file("env.txt");

    std::string row;
    std::string dataType;
    std::getline(file, dataType);
    std::getline(file, row);
    
    if (dataType == "gb") {
        return std::stoll(row) * 1073741824;
    } else if (dataType == "byte") {
        return std::stoll(row);
    }
    return 0;
}

long long getChunkSize() {
    std::ifstream file("env.txt");

    std::string row;
    std::string dataType;
    std::getline(file, dataType);
    std::getline(file, row);
    std::getline(file, dataType);
    std::getline(file, row);

    if (dataType == "gb") {
        return std::stoll(row) * 1073741824;
    } else if (dataType == "byte") {
        return std::stoll(row);
    }
    return 0;
}

int getNThreads() {
    std::ifstream file("env.txt");

    std::string row;
    std::getline(file, row);
    std::getline(file, row);
    std::getline(file, row);
    std::getline(file, row);
    std::getline(file, row);

    return std::stoi(row);
}

long long FILE_BYTE_SIZE = getFileByteSize();
long long CHUNK_SIZE = getChunkSize();
int N_THREADS = getNThreads();
long long BYTE_EACH_THREAD{FILE_BYTE_SIZE / N_THREADS};
long long LAST_THREAD_BYTES{FILE_BYTE_SIZE / N_THREADS + FILE_BYTE_SIZE % N_THREADS};

void assignFileToThread(std::string n, long long bytes, bool last = false) {
    std::ofstream file("temp_" + n + ".txt", std::ios::binary | std::ios::app);
    if (!file) {
        std::cerr << "File error\n";
        return exit(1);
    }

    LongKeyChunk chunker{};
    if (!last) {
        chunker = LongKeyChunk(BYTE_EACH_THREAD, CHUNK_SIZE);
    } else {
        chunker = LongKeyChunk(LAST_THREAD_BYTES, CHUNK_SIZE);
    }

    while (true) {
        auto data = chunker.get();

        if (data) {
            file.write(reinterpret_cast<const char*>((*data).data()),
               static_cast<std::streamsize>((*data).size()));
        } else break;
    }
}

void deleteFile(std::filesystem::path p) {
    std::error_code ec;
    std::filesystem::remove(p, ec);

    if (ec) {
        std::cerr << "Error while deleting " << p << ": " << ec.message() << '\n';
        exit(1);
    }
}

void createFinalFile() {
    std::cout << "\nLinking..." << "\n";

    std::ofstream file("key.txt", std::ios::binary | std::ios::app);
    if (!file) {
        std::cerr << "File error\n";
        return exit(1);
    }

    for (int i = 0; i < N_THREADS; ++i) {
        std::string fileName = "temp_" + std::to_string(i) + ".txt";
        std::cout << "Linking " << fileName << "\n";

        std::ifstream in(fileName, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Can't open " + fileName);
    }

    std::vector<char> buffer(4 * 1024 * 1024); // Sempre e solo 4 MB di RAM
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        std::streamsize bytesRead = in.gcount();
        if (bytesRead > 0) {
            file.write(buffer.data(), bytesRead);
        }
    }
    in.close();
    deleteFile(fileName);
    }

    std::cout << "Key completed!\n";
}

int main() {
    std::cout << "STARTED KEY_MAKING\n";
    std::cout << "vars:\n";
    std::cout << "     " << "FILE_BYTE_SIZE=" << FILE_BYTE_SIZE << "\n"; 
    std::cout << "     " << "CHUNK_SIZE=" << CHUNK_SIZE << "\n"; 
    std::cout << "     " << "N_THREADS=" << N_THREADS << "\n";
    std::cout << "     " << "BYTE_EACH_THREAD=" << BYTE_EACH_THREAD << "\n";
    std::cout << "     " << "LAST_THREAD_BYTES=" << LAST_THREAD_BYTES << "\n";

    std::cout << "\n";

    std::thread* threadPool = new std::thread[N_THREADS];

    for (int i = 0; i < N_THREADS - 1; ++i) {
        std::cout << "Starting thread #" << std::to_string(i) << "\n";
        threadPool[i] = std::thread(assignFileToThread, std::to_string(i), BYTE_EACH_THREAD, false);
    }
    std::cout << "Starting thread #" << std::to_string(N_THREADS - 1) << "\n";
    threadPool[N_THREADS - 1] = std::thread(assignFileToThread, std::to_string(N_THREADS - 1), BYTE_EACH_THREAD, true);

    std::cout << "\n";
    for (int i =0; i < N_THREADS; ++i) {
        std::cout << "Joining thread #" << std::to_string(i) << "\n";
        threadPool[i].join();
    }

    createFinalFile();
    delete[] threadPool;
    system("pause");
}