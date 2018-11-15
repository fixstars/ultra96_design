#include <cstdint>
#include "Halide.h"
#include <Element.h>
#include <math.h>

namespace HalideISP {

using namespace Halide;
using namespace Halide::Element;

class Demosaic : public Halide::Generator<Demosaic> {

public:
  ImageParam idata{UInt(10), 2, "idata"};
  ImageParam ilast{type_of<bool>(), 2, "ilast"};
  ImageParam iuser{type_of<bool>(), 2, "iuser"};

  GeneratorParam<int32_t> width{"width", 640};
  GeneratorParam<int32_t> height{"height", 480};

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
      out(c, x, y) = cast<uint8_t>(select(c == 0, r_expr,
                                          c == 1, g_expr,
                                                  b_expr) >> 2);

      return out;
  }

  Pipeline build() {
    Var c{"c"};
    Var x{"x"};
    Var y{"y"};
    Func odata{"odata"};
    Func olast{"olast"};
    Func ouser{"ouser"};

    odata(c, x, y) = demosaic(idata, width, height)(c, x, y);

    olast(x, y) = consume(ilast(x/4, y)) || (x == width-1);
    ouser(x, y) = consume(iuser(x/4, y)) || (x == 0 && y == 0);

    schedule(ilast, {width/4, height})
        .hls_interface(Interface::Type::Stream, (width/4)*2);
    schedule(iuser, {width/4, height})
        .hls_interface(Interface::Type::Stream, (width/4)*2);
    schedule(idata, {width, height})
        .hls_burst(4)
        .hls_interface(Interface::Type::Stream)
        .hls_bundle(Interface::Signal::LAST, ilast)
        .hls_bundle(Interface::Signal::USER, iuser);

    schedule(olast, {width, height})
        .hls_interface(Interface::Type::Stream);
    schedule(ouser, {width, height})
        .hls_interface(Interface::Type::Stream);
    schedule(odata, {3, width, height})
        .unroll(c)
        .hls_burst(3)
        .hls_interface(Interface::Type::Stream)
        .hls_bundle(Interface::Signal::LAST, olast)
        .hls_bundle(Interface::Signal::USER, ouser);

    return Pipeline({odata, olast, ouser});
  }
};

}

HALIDE_REGISTER_GENERATOR(HalideISP::Demosaic, demosaic)
