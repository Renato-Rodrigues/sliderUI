// Simple unit test for file_utils
#include "core/file_utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unistd.h>

int main() {
    std::cout << "Running file_utils test...\n";
    std::string tmpdir = "/tmp";
    pid_t pid = getpid();
    std::string test_path = tmpdir + "/sliderui_test_file_utils_" + std::to_string(pid) + ".txt";

    // Ensure removal on exit
    auto cleanup = [&](void){
        unlink(test_path.c_str());
    };

    std::string content = "Hello atomic world!\nLine2\n";
    bool w = file_utils::atomic_write(test_path, content);
    if (!w) {
        std::cerr << "[FAIL] atomic_write returned false\n";
        cleanup();
        return 1;
    }

    if (!file_utils::file_exists(test_path)) {
        std::cerr << "[FAIL] file_exists returned false after atomic_write\n";
        cleanup();
        return 2;
    }

    // read back and check contents
    std::ifstream in(test_path, std::ios::binary);
    if (!in) {
        std::cerr << "[FAIL] open for read failed\n";
        cleanup();
        return 3;
    }
    std::string readback((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    if (readback != content) {
        std::cerr << "[FAIL] readback mismatch\n";
        std::cerr << "expected:\n" << content << "\n--- got:\n" << readback << "\n";
        cleanup();
        return 4;
    }

    uint64_t mtime = file_utils::file_mtime(test_path);
    if (mtime == 0) {
        std::cerr << "[FAIL] file_mtime returned 0\n";
        cleanup();
        return 5;
    }

    // Test overwriting (atomic replace)
    std::string content2 = "Second content\n";
    bool w2 = file_utils::atomic_write(test_path, content2);
    if (!w2) {
        std::cerr << "[FAIL] atomic_write overwrite returned false\n";
        cleanup();
        return 6;
    }
    // read back
    std::ifstream in2(test_path, std::ios::binary);
    std::string read2((std::istreambuf_iterator<char>(in2)), std::istreambuf_iterator<char>());
    in2.close();
    if (read2 != content2) {
        std::cerr << "[FAIL] overwrite readback mismatch\n";
        cleanup();
        return 7;
    }

    cleanup();
    std::cout << "[OK] test_file_utils passed\n";
    return 0;
}
