#pragma once

template<typename argT = char>
struct args_view {
    args_view(int argc, argT** argv) : argc(argc), argv(argv) {}

    int argc;
    argT** argv;

    std::basic_string_view<argT> operator[](int index) const {
        if (index < 0 || index >= argc) {
            throw std::out_of_range("index out of range");
        }
        return argv[index];
    }

    auto begin() const {
        return argv;
    }

    auto end() const {
        return argv + argc;
    }
};
