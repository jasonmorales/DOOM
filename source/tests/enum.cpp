#include "nstd/utility/enum.h"

import nstd.numbers;
import nstd.enum_ref;

ENUM(EA, int32, zxcv);
ENUM(EB, int32, asdf, qwerty, uiop);
ENUM(EC, int32, AA=3, BB, CC = -8, DD);

bool test_enum()
{
    //*
    std::cout << EC::type_name() << "[" << EC::size() << "]\n";
    for (int n = 0; n < EC::size(); ++n)
        std::cout << EC::name_from_index(n) << " = " << static_cast<int>(EC::value_from_index(n).value()) << "\n";

    std::cout << "\n";

    constexpr std::array<EC, 4> vals = {EC::AA, EC::BB, EC::CC, EC::DD};
    for (auto& val : vals)
        std::cout << EC::index(val) << ": " << EC::name(val) << "\n";

    std::cout << "\n";

    for (auto& val : vals)
        std::cout << val.index() << ": " << val.name() << " = " << static_cast<int>(val) << "\n";

    std::cout << "\n";

    EC x = EC::from_index(0).value();
    EC y = EC::value("DD").value();
    EC z = EC::first();
    for (int n = 0; n < EC::size(); ++n, ++x, --y)
        std::cout << x.name() << " | " << y.name() << " | " << static_cast<int>(z + n) << "\n";

    std::cout << "\n";

    for (auto e : EC::set)
    {
        std::cout << e.name() << "\n";
    }

    std::cout << std::endl;
    /**/
    return true;
}
