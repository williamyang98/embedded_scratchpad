#include "./utility/cobs.hpp"
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <format>
#include <string>
#include <span>

static void print_buffer(FILE* fp, std::span<const uint8_t> buffer) {
    const size_t N = buffer.size();
    fprintf(fp, "{");
    for (size_t i = 0; i < N; i++) {
        fprintf(fp, "0x%.02X", buffer[i]);
        if (i != N-1) fprintf(fp, ",");
    }
    fprintf(fp, "}");
}

static bool test_compare_array(const std::string& label, std::span<const uint8_t> expected, std::span<const uint8_t> given) {
    FILE* fp = stderr;
    if (expected.size() != given.size()) {
        fprintf(fp, "FAILED TEST: %s\n", label.c_str());
        fprintf(fp, "  mismatching array lengths %zu != %zu\n", expected.size(), given.size());
        fprintf(fp, "  expected[%zu]=", expected.size()); print_buffer(fp, expected); fprintf(fp, "\n");
        fprintf(fp, "  given[%zu]   =", given.size()); print_buffer(fp, given); fprintf(fp, "\n");
        return false;
    }

    bool is_mismatch = false;
    const size_t N = expected.size();
    for (size_t i = 0; i < N; i++) {
        if (expected[i] == given[i])  continue;
        if (!is_mismatch) {
            fprintf(fp, "FAILED TEST: %s\n", label.c_str());
        }
        fprintf(fp, "  mismatching byte at i=%zu, expected[%zu]=%u, given[%zu]=%u\n",
            i,
            i, expected[i],
            i, given[i]
        );
        is_mismatch = true;
    }

    if (is_mismatch) {
        fprintf(fp, "  expected[%zu]=", expected.size()); print_buffer(fp, expected); fprintf(fp, "\n");
        fprintf(fp, "  given[%zu]   =", given.size()); print_buffer(fp, given); fprintf(fp, "\n");
    }
    return !is_mismatch;
}

struct TestCase {
    std::vector<uint8_t> src;
    std::vector<uint8_t> dest;
};

static bool run_test_case(const std::string& label, std::span<const uint8_t> expected_src_buffer, std::span<const uint8_t> expected_dest_buffer) {
    bool is_passed = true;
    {
        std::vector<uint8_t> given_dest_buffer;
        given_dest_buffer.resize(cobs::get_maximum_encoded_size(expected_src_buffer.size()));
        const size_t given_dest_size = cobs::encode(
            expected_src_buffer.data(), expected_src_buffer.size(),
            given_dest_buffer.data()
        );
        is_passed &= test_compare_array(
            std::format("{0} - Compare encoding", label),
            expected_dest_buffer, std::span(given_dest_buffer).first(given_dest_size)
        );
    }
    {
        std::vector<uint8_t> given_src_buffer;
        given_src_buffer.resize(cobs::get_maximum_decoded_size(expected_dest_buffer.size()));
        const size_t given_src_size = cobs::decode(
            expected_dest_buffer.data(), expected_dest_buffer.size(),
            given_src_buffer.data()
        );
        is_passed &= test_compare_array(
            std::format("{0} - Compare decoding", label),
            expected_src_buffer, std::span(given_src_buffer).first(given_src_size)
        );
    }

    if (is_passed) {
        printf("Passed test: %s\n", label.c_str());
    } else {
        printf("Failed test: %s\n", label.c_str());
    }
    return is_passed;
}

static bool run_test_case(const std::string& label, const TestCase& test_case) {
    return run_test_case(label, test_case.src, test_case.dest);
}

struct TestCounter {
    size_t total_tests = 0;
    size_t total_failed = 0;
    size_t total_passed = 0;
    void operator+=(bool is_passed) {
        if (is_passed) {
            total_passed++;
        } else {
            total_failed++;
        }
        total_tests++;
    }
};

int main(int argc, char** argv) {
    TestCounter counter;
    // https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing#Encoding_examples
    counter += run_test_case("Example 1", TestCase {{ 0x00 }, { 0x01, 0x01, 0x00 }});
    counter += run_test_case("Example 2", TestCase {{ 0x00, 0x00 }, { 0x01, 0x01, 0x01, 0x00 }});
    counter += run_test_case("Example 3", TestCase {{ 0x00, 0x11, 0x00 }, { 0x01, 0x02, 0x11, 0x01, 0x00 }});
    counter += run_test_case("Example 4", TestCase {{ 0x11, 0x22, 0x00, 0x33 }, { 0x03, 0x11, 0x22, 0x02, 0x33, 0x00 }});
    counter += run_test_case("Example 5", TestCase {{ 0x11, 0x22, 0x33, 0x44 }, { 0x05, 0x11, 0x22, 0x33, 0x44, 0x00 }});
    counter += run_test_case("Example 6", TestCase {{ 0x11, 0x00, 0x00, 0x00 }, { 0x02, 0x11, 0x01, 0x01, 0x01, 0x00 }});
    {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 0x01; i <= 0xFE; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }

        dest.push_back(0xFF);
        for (size_t i = 0x01; i <= 0xFE; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest.push_back(0x00);

        counter += run_test_case("Example 7", src, dest);
    }
    {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 0x00; i <= 0xFE; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }

        dest.push_back(0x01);
        dest.push_back(0xFF);
        for (size_t i = 0x01; i <= 0xFE; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest.push_back(0x00);

        counter += run_test_case("Example 8", src, dest);
    }
    {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 0x01; i <= 0xFF; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }

        dest.push_back(0xFF);
        for (size_t i = 0x01; i <= 0xFE; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest.push_back(0x02);
        dest.push_back(0xFF);
        dest.push_back(0x00);

        counter += run_test_case("Example 9", src, dest);
    }
    {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 0x02; i <= 0xFF; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }
        src.push_back(0x00);

        dest.push_back(0xFF);
        for (size_t i = 0x02; i <= 0xFF; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest.push_back(0x01);
        dest.push_back(0x01);
        dest.push_back(0x00);

        counter += run_test_case("Example 10", src, dest);
    }
    {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 0x03; i <= 0xFF; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }
        for (size_t i = 0x00; i <= 0x01; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }

        dest.push_back(0xFE);
        for (size_t i = 0x03; i <= 0xFF; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest.push_back(0x02);
        dest.push_back(0x01);
        dest.push_back(0x00);

        counter += run_test_case("Example 11", src, dest);
    }
    {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 0; i <= 253; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }

        dest.push_back(0x01);
        dest.push_back(0xFE);
        for (size_t i = 1; i <= 253; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest.push_back(0x00);

        counter += run_test_case("Example A1", src, dest);
    }
    {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 3; i <= 254; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }
        src.push_back(0x00);
        src.push_back(0x01);

        dest.push_back(0xFD);
        for (size_t i = 3; i <= 254; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest.push_back(0x02);
        dest.push_back(0x01);
        dest.push_back(0x00);

        counter += run_test_case("Example A2", src, dest);
    }
    for (uint8_t zero_index = 0; zero_index < 254; zero_index++) {
        std::vector<uint8_t> src;
        std::vector<uint8_t> dest;
        for (size_t i = 1; i <= 254; i++) {
            src.push_back(static_cast<uint8_t>(i));
        }
        src[zero_index] = 0;

        dest.push_back(zero_index+1);
        for (size_t i = 1; i <= 254; i++) {
            dest.push_back(static_cast<uint8_t>(i));
        }
        dest[zero_index+1] = 254-zero_index;
        dest.push_back(0x00);

        counter += run_test_case(std::format("Example A3 - zero index {0}", zero_index), src, dest);
    }

    printf("total_tests=%zu, passed=%zu, failed=%zu\n", counter.total_tests, counter.total_passed, counter.total_failed);

    return 0;
}
