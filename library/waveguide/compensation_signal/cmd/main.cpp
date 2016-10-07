#include "compensation_signal/waveguide.h"

#include "utilities/progress_bar.h"

#include <iomanip>
#include <iostream>

/// This is a short program to find the impulse response of a rectangular
/// waveguide mesh with the maximum permissable courant number.
/// This signal will be written to a file called 'mesh_impulse_response.wav'.

/// There is a tool in the `data` folder to write the signal into a .cpp source
/// so that it can be baked into the waveguide library.
/// But, because this might take a long time to run, I've decided to just
/// include a copy of the .wav in the repository.

/// This tool is included mainly so that you can see how the mesh ir is
/// generated, and to give you the option of generating a longer ir if you
/// decide that is necessary.

template <typename It>
void generate_data_file(std::ostream& os, It begin, It end) {
    const auto elements{std::distance(begin, end)};

    os << "//  Autogenerated file  //\n";
    os << "#pragma once\n";
    os << "#include <array>\n";
    os << "const std::array<float, " << elements
       << "> mesh_impulse_response{{\n";

    for (auto count{0ul}; begin != end; ++begin, ++count) {
        os << std::setprecision(std::numeric_limits<float>::max_digits10)
           << *begin << ", ";
        if (count && !(count % 8)) {
            os << "\n";
        }
    }

    os << "}};\n";
}

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error{"expected a step number"};
    }

    const size_t steps = std::stoi(argv[1]);

    //  get a cl context and device
    const compute_context cc{};

    //  Creates a compressed waveguide with the specified centre-to-edge
    //  distance. For a distance n, this can be used to generate a mesh ir with
    //  length 2n.
    compressed_rectangular_waveguide waveguide{cc, steps};

    //  run the waveguide with an impulsive hard source
    progress_bar pb{std::cerr, steps};
    const std::vector<float> sig{0.0f, 1.0f};
    const auto output{waveguide.run_hard_source(
            sig.begin(), sig.end(), [&](auto step) { pb += 1; })};

    //  write a cpp source file
    generate_data_file(std::cout, output.begin(), output.end());
}