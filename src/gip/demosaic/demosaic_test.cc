#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "demosaic.h"
#include "test_common.h"

#include "HalideRuntime.h"

using std::string;
using std::vector;

using namespace Halide::Runtime;

template<typename T>
Buffer<T> load_bin(std::string fname)
{
    std::ifstream ifs(fname.c_str(), std::ios_base::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error(format("File not found : %s", fname.c_str()));
    }

    uint16_t w, h;
    ifs.read(reinterpret_cast<char*>(&w), sizeof(w));
    ifs.read(reinterpret_cast<char*>(&h), sizeof(h));

    // remaining size
    ifs.seekg(0, std::ifstream::end);
    std::ifstream::pos_type end = ifs.tellg();

    ifs.seekg(2*sizeof(uint16_t), std::ifstream::beg);
    std::ifstream::pos_type beg = ifs.tellg();

    std::size_t buf_size_in_byte = end-beg;

    auto buffer = mk_null_buffer<T>({static_cast<int32_t>(w), static_cast<int32_t>(h)});

    ifs.read(reinterpret_cast<char*>(buffer.data()), buf_size_in_byte);

    return buffer;
}

template<typename T>
void save_ppm(const std::string& fname, const Buffer<T>& buffer, int32_t width, int32_t height)
{
    std::ofstream ofs(fname);
    ofs << "P6" << std::endl;
    ofs << width << " " << height << std::endl;
    ofs << static_cast<int>(std::numeric_limits<T>::max()) << std::endl;
    ofs.write(reinterpret_cast<const char *>(buffer.data()), buffer.size_in_bytes());
}

int main(int argc, char *argv[])
{
    try {
        int ret = 0;

        const int width = 1920;
        const int height = 1080;

        const std::vector<int32_t> extents{width, height};
        auto input = load_bin<uint16_t>(argv[1]);
        auto ilast = mk_null_buffer<bool>({width/4, height});
        auto iuser = mk_null_buffer<bool>({width/4, height});
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width/4; ++x) {
                ilast(x, y) = (x == width/4-1);
                iuser(x, y) = x == 0 && y == 0;
            }
        }

        auto output = mk_null_buffer<uint8_t>({3, width, height});
        auto olast = mk_null_buffer<bool>({width, height});
        auto ouser = mk_null_buffer<bool>({width, height});

        demosaic(input, ilast, iuser, output, olast, ouser);

        save_ppm("out.ppm", output, width, height);

        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                if (olast(x, y) != (x == width-1)) {
                    throw std::runtime_error(format("expect:%u, actual:%u\n", (x == width-1), olast(x, y)));
                }
                if (ouser(x, y) != (x == 0 && y == 0)) {
                    throw std::runtime_error(format("expect:%u, actual:%u\n", (x == 0 && y == 0), ouser(x, y)));
                }
            }
        }


    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    printf("Success!\n");
    return 0;
}
