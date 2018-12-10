#include <cstdint>
#include "Halide.h"
#include <Element.h>
#include <math.h>

#define YUYVOUT

namespace HalideISP {

using namespace Halide;
using namespace Halide::Element;

class Demosaic : public Halide::Generator<Demosaic> {

public:
    ImageParam idata{UInt(10), 2, "idata"};
    ImageParam ilast{type_of<bool>(), 2, "ilast"};
    ImageParam iuser{type_of<bool>(), 2, "iuser"};

    const int w = 640, h = 480;
    #ifdef YUYVOUT
    GeneratorParam<int32_t> in_width{"in_width", w};
    GeneratorParam<int32_t> width{"width", w / 2};
    GeneratorParam<int32_t> height{"height", h};
    #else /* !YUYVOUT */
    GeneratorParam<int32_t> in_width{"in_width", w};
    GeneratorParam<int32_t> width{"width", w};
    GeneratorParam<int32_t> height{"height", h};
    #endif /* !YUYVOUT */

    // Input is RAW10 and output is RGB888
    Func demosaic(Func in_, int32_t width, int32_t height)
    {
        Var c, x, y;
        Func in;
        in(x, y) = cast<uint16_t>(in_(x, y));
        in = BoundaryConditions::constant_exterior(in, cast<uint16_t>(0), 0, width, 0, height);

        Func out("demosaic");
        Expr self = in(x, y);
        Expr horizontal = (in(x-1, y) + in(x+1, y)) / static_cast<uint16_t>(2);
        Expr vertical   = (in(x, y-1) + in(x, y+1)) / static_cast<uint16_t>(2);
        Expr diagonal = (in(x-1, y-1) + in(x+1, y-1) + in(x-1, y+1) + in(x+1, y+1)) / static_cast<uint16_t>(4);
        Expr lattice  = (in(x-1, y)   + in(x+1, y)   + in(x,   y-1) + in(x,   y+1)) / static_cast<uint16_t>(4);

        Expr is_r  = ((x % 2) == 0) && ((y % 2) == 0);
        Expr is_gr = ((x % 2) == 1) && ((y % 2) == 0);
        Expr is_gb = ((x % 2) == 0) && ((y % 2) == 1);
        Expr is_b  = ((x % 2) == 1) && ((y % 2) == 1);

        Expr g_expr = select(is_r,  self,
                             is_gr, horizontal,
                             is_gb, vertical,
                             diagonal);

        Expr r_expr = select(is_r,  lattice,
                             is_gr, self,
                             is_gb, self,
                             lattice);

        Expr b_expr = select(is_r,  diagonal,
                             is_gr, vertical,
                             is_gb, horizontal,
                             self);

        // Shift out least significant 2-bit to downcast to 8-bit unsigned representation
        out(c, x, y) = cast<uint8_t>(select(c == 0, g_expr,
                                            c == 1, r_expr,
                                            b_expr) >> 2);

        return out;
    }

    #ifdef YUYVOUT
    // Input is RGB888 and output is YUYV
    Func rgbtoyuv(Func src)
    {
        Var c, x, y;
        Func out{"rgbtoyuv"};
        const float k1[3][4] = {
            { 0.5f,	      0.29891f, -0.16874f, 0.0f},
            {-0.418691f,  0.58661f, -0.33126f, 0.0f},
            {-0.08131f,   0.11448f,  0.5f,	   0.0f}
        };

        Expr Y0 = k1[0][1] * src(0, x * 2, y)
            + k1[1][1] * src(1, x * 2, y)
            + k1[2][1] * src(2, x * 2, y);
        Expr U = k1[0][2] * src(0, x * 2, y)
            + k1[1][2] * src(1, x * 2, y)
            + k1[2][2] * src(2, x * 2, y) + 128;
        Expr V = k1[0][0] * src(0, x * 2, y)
            + k1[1][0] * src(1, x * 2, y)
            + k1[2][0] * src(2, x * 2, y) + 128;
        Expr Y1 = k1[0][1] * src(0, x * 2 + 1, y)
            + k1[1][1] * src(1, x * 2 + 1, y)
            + k1[2][1] * src(2, x * 2 + 1, y);
        out(c, x, y) = cast<uint8_t>(clamp(select(c == 0, Y0,
                                                  c == 1, U,
                                                  c == 2, Y1,
                                                  V), 0, 255));

        return out;
    }

    Func rgbtoyuv_fixed(Func src)
    {
        constexpr uint32_t FB = 10;
        using base_t = int32_t;
        using Fixed32 = Fixed<int32_t, FB>;

        Var c, x, y;
        Func out{"rgbtoyuv_fixed"};
        const Fixed32 k1[3][4] = {
            { Fixed32::to_fixed(0.5f),      Fixed32::to_fixed(0.29891f), Fixed32::to_fixed(-0.16874f), Fixed32::to_fixed(0.0f)},
            {Fixed32::to_fixed(-0.418691f), Fixed32::to_fixed(0.58661f), Fixed32::to_fixed(-0.33126f), Fixed32::to_fixed(0.0f)},
            {Fixed32::to_fixed(-0.08131f),  Fixed32::to_fixed(0.11448f), Fixed32::to_fixed(0.5f),	   Fixed32::to_fixed(0.0f)}
        };

        Expr Y0 = from_fixed<base_t>(k1[0][1] * Fixed32::to_fixed(src(0, x * 2, y)) + k1[1][1] * Fixed32::to_fixed(src(1, x * 2, y)) + k1[2][1]  * Fixed32::to_fixed(src(2, x * 2, y)));
        Expr U = from_fixed<base_t>(k1[0][2] * Fixed32::to_fixed(src(0, x * 2, y)) + k1[1][2] * Fixed32::to_fixed(src(1, x * 2, y)) + k1[2][2] * Fixed32::to_fixed(src(2, x * 2, y)) + to_fixed<int32_t, FB>(128));
        Expr V = from_fixed<base_t>(k1[0][0] * Fixed32::to_fixed(src(0, x * 2, y)) + k1[1][0] * Fixed32::to_fixed(src(1, x * 2, y)) + k1[2][0] * Fixed32::to_fixed(src(2, x * 2, y)) + to_fixed<int32_t, FB>(128));
        Expr Y1 = from_fixed<base_t>(k1[0][1] * Fixed32::to_fixed(src(0, x * 2 + 1, y)) + k1[1][1] * Fixed32::to_fixed(src(1, x * 2 + 1, y)) + k1[2][1] * Fixed32::to_fixed(src(2, x * 2 + 1, y)));
        out(c, x, y) = cast<uint8_t>(clamp(select(c == 0, Y0,
                                                  c == 1, U,
                                                  c == 2, Y1,
                                                  V), 0, 255));

        return out;
    }
    #endif /* YUYVOUT */

    Pipeline build()
    {
        Var c{"c"};
        Var x{"x"};
        Var y{"y"};
        Func odata{"odata"};
        Func olast{"olast"};
        Func ouser{"ouser"};
        #ifdef YUYVOUT
        const int channel = 4;
        #else /* !YUYVOUT */
        const int channel = 3;
        #endif /* !YUYVOUT */

        #ifdef YUYVOUT
        Func dmc{"demosaic"};
        dmc(c, x, y) = demosaic(idata, in_width, height)(c, x, y);
        odata(c, x, y) = rgbtoyuv_fixed(dmc)(c, x, y);
        #else /* !YUYVOUT */
        odata(c, x, y) = demosaic(idata, in_width, height)(c, x, y);
        #endif /* !YUYVOUT */

        olast(x, y) = consume(ilast(x/4, y)) || (x == width-1);
        ouser(x, y) = consume(iuser(x/4, y)) || (x == 0 && y == 0);

        schedule(ilast, {in_width/4, height})
            .hls_interface(Interface::Type::Stream, (in_width/4)*2);
        schedule(iuser, {in_width/4, height})
            .hls_interface(Interface::Type::Stream, (in_width/4)*2);
        schedule(idata, {in_width, height})
            .hls_burst(4)
            .hls_interface(Interface::Type::Stream)
            .hls_bundle(Interface::Signal::LAST, ilast)
            .hls_bundle(Interface::Signal::USER, iuser);

        schedule(olast, {width, height})
            .hls_interface(Interface::Type::Stream);
        schedule(ouser, {width, height})
            .hls_interface(Interface::Type::Stream);
        schedule(odata, {channel, width, height})
            .unroll(c)
            .hls_burst(channel)
            .hls_interface(Interface::Type::Stream)
            .hls_bundle(Interface::Signal::LAST, olast)
            .hls_bundle(Interface::Signal::USER, ouser);
        #ifdef YUYVOUT
        schedule(dmc, {3, in_width, height});
        #endif /* YUYVOUT */

        return Pipeline({odata, olast, ouser});
    }
};

}

HALIDE_REGISTER_GENERATOR(HalideISP::Demosaic, demosaic)
