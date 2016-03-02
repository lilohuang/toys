#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <memory>

#include "config.h"
#include "../../gettime.cpp"
#include "../../cmdline.cpp"
#include "../../fnv32.cpp"

#include "encode.scalar.cpp"
#include "lookup.sse.cpp"
#include "encode.sse.cpp"

#include "application.cpp"

class Application final: public ApplicationBase {

public:
    Application(const CommandLine& c)
        : ApplicationBase(c) {}

    int run() {
        if (cmd.empty() || cmd.has("scalar32")) {
            measure("scalar32", base64::scalar::encode32);
        }

        if (cmd.empty() || cmd.has("scalar64")) {
            measure("scalar64", base64::scalar::encode64);
        }

        auto sse_naive = [](uint8_t* input, size_t bytes, uint8_t* output) {
            base64::sse::encode(base64::sse::lookup_naive, input, bytes, output);
        };

        auto sse_optimized1 = [](uint8_t* input, size_t bytes, uint8_t* output) {
            base64::sse::encode(base64::sse::lookup_version1, input, bytes, output);
        };

        auto sse_optimized2 = [](uint8_t* input, size_t bytes, uint8_t* output) {
            base64::sse::encode(base64::sse::lookup_version2, input, bytes, output);
        };

        if (cmd.empty() || cmd.has("sse")) {
            measure("SSE (naive)", sse_naive);
        }

        if (cmd.empty() || cmd.has("sse1")) {
            measure("SSE (optimized v1)", sse_optimized1);
        }

        if (cmd.empty() || cmd.has("sse2")) {
            measure("SSE (optimized v2)", sse_optimized2);
        }

#if defined(HAVE_BMI2_INSTRUCTIONS)
        auto sse_bmi2_naive = [](uint8_t* input, size_t bytes, uint8_t* output) {
            base64::sse::encode_bmi2(base64::sse::lookup_naive, input, bytes, output);
        };

        auto sse_bmi2_optimized = [](uint8_t* input, size_t bytes, uint8_t* output) {
            base64::sse::encode_bmi2(base64::sse::lookup_version1, input, bytes, output);
        };

        if (cmd.empty() || cmd.has("bmi1")) {
            measure("SSE & BMI2 (naive)", sse_bmi2_naive);
        }

        if (cmd.empty() || cmd.has("bmi2")) {
            measure("SSE & BMI2 (optimized)", sse_bmi2_optimized);
        }
#endif

        return 0;
    }

    template<typename T>
    double measure(const char* name, T callback) {

        initialize();

        printf("%s... ", name);
        fflush(stdout);

        unsigned n = iterations;
        double time = -1;
        while (n-- > 0) {

            const auto t1 = get_time();
            callback(input.get(), get_input_size(), output.get());
            const auto t2 = get_time();

            const double t = (t2 - t1)/1000000.0;
            if (time < 0) {
                time = t;
            } else {
                time = std::min(time, t);
            }
        }

        printf("%0.3f\n", time);
        return time;
    }
};


int main(int argc, char* argv[]) {

    CommandLine cmd(argc, argv);
    Application app(cmd);

    return app.run();
}
