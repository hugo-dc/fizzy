#include "parser.hpp"
#include "utils.hpp"
#include <gtest/gtest.h>

using namespace fizzy;

static const auto functype_void_to_void = from_hex("600000");
static const auto functype_i32i64_to_i32 = from_hex("60027f7e017f");
static const auto functype_i32_to_void = from_hex("60017f00");

TEST(parser, valtype)
{
    uint8_t b;
    b = 0x7e;
    EXPECT_EQ(std::get<0>(parser<valtype>{}(&b)), valtype::i64);
    b = 0x7f;
    EXPECT_EQ(std::get<0>(parser<valtype>{}(&b)), valtype::i32);
    b = 0x7d;
    EXPECT_THROW(std::get<0>(parser<valtype>{}(&b)), parser_error);
}

TEST(parser, valtype_vec)
{
    const auto input = from_hex("037f7e7fcc");
    const auto [vec, pos] = parser<std::vector<valtype>>{}(input.data());
    EXPECT_EQ(pos, input.data() + 4);
    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], valtype::i32);
    EXPECT_EQ(vec[1], valtype::i64);
    EXPECT_EQ(vec[2], valtype::i32);
}

TEST(parser, locals)
{
    const auto input = from_hex("81017f");
    const auto [l, p] = parser<locals>{}(input.data());
    EXPECT_EQ(l.count, 0x81);
    EXPECT_EQ(l.type, valtype::i32);
}

TEST(parser, empty_module)
{
    const auto module = parse(wasm_prefix);
    EXPECT_EQ(module.typesec.size(), 0);
    EXPECT_EQ(module.funcsec.size(), 0);
    EXPECT_EQ(module.codesec.size(), 0);
}

TEST(parser, module_with_wrong_prefix)
{
    EXPECT_THROW(parse({}), parser_error);
    EXPECT_THROW(parse(bytes{0x00, 0x61, 0x73, 0xd6}), parser_error);
    EXPECT_THROW(parse(bytes{0x00, 0x61, 0x73, 0xd6, 0x00, 0x00, 0x00, 0x00}), parser_error);
    EXPECT_THROW(parse(bytes{0x00, 0x61, 0x73, 0xd6, 0x02, 0x00, 0x00, 0x00}), parser_error);
}

TEST(parser, custom_section_empty)
{
    const auto bin = bytes{wasm_prefix} + from_hex("0000");
    const auto module = parse(bin);
    EXPECT_EQ(module.typesec.size(), 0);
    EXPECT_EQ(module.funcsec.size(), 0);
    EXPECT_EQ(module.codesec.size(), 0);
}

TEST(parser, custom_section_nonempty)
{
    const auto bin = bytes{wasm_prefix} + from_hex("0001ff");
    const auto module = parse(bin);
    EXPECT_EQ(module.typesec.size(), 0);
    EXPECT_EQ(module.funcsec.size(), 0);
    EXPECT_EQ(module.codesec.size(), 0);
}

TEST(parser, functype_wrong_prefix)
{
    const auto section_contents = uint8_t{0x01} + from_hex("610000");
    const auto bin =
        bytes{wasm_prefix} + uint8_t{0x01} + uint8_t(section_contents.size()) + section_contents;
    EXPECT_THROW(parse(bin), parser_error);
}

TEST(parser, type_section_larger_than_expected)
{
    const auto section_contents = uint8_t{0x01} + functype_void_to_void;
    const auto bin = bytes{wasm_prefix} + uint8_t{0x01} + uint8_t(section_contents.size() - 1) +
                     section_contents;
    EXPECT_THROW(parse(bin), parser_error);
}

TEST(parser, type_section_smaller_than_expected)
{
    const auto section_contents = uint8_t{0x01} + functype_void_to_void + uint8_t{0xfe};
    const auto bin =
        bytes{wasm_prefix} + uint8_t{0x01} + uint8_t(section_contents.size()) + section_contents;
    EXPECT_THROW(parse(bin), parser_error);
}

TEST(parser, type_section_with_single_functype)
{
    // single type [void] -> [void]
    const auto section_contents = uint8_t{0x01} + functype_void_to_void;
    const auto bin =
        bytes{wasm_prefix} + uint8_t{0x01} + uint8_t(section_contents.size()) + section_contents;
    const auto module = parse(bin);
    ASSERT_EQ(module.typesec.size(), 1);
    const auto functype = module.typesec[0];
    EXPECT_EQ(functype.inputs.size(), 0);
    EXPECT_EQ(functype.outputs.size(), 0);
    EXPECT_EQ(module.funcsec.size(), 0);
    EXPECT_EQ(module.codesec.size(), 0);
}

TEST(parser, type_section_with_single_functype_params)
{
    // single type [i32, i64] -> [i32]
    const auto section_contents = uint8_t{0x01} + functype_i32i64_to_i32;
    const auto bin =
        bytes{wasm_prefix} + uint8_t{0x01} + uint8_t(section_contents.size()) + section_contents;
    const auto module = parse(bin);
    ASSERT_EQ(module.typesec.size(), 1);
    const auto functype = module.typesec[0];
    ASSERT_EQ(functype.inputs.size(), 2);
    EXPECT_EQ(functype.inputs[0], valtype::i32);
    EXPECT_EQ(functype.inputs[1], valtype::i64);
    ASSERT_EQ(functype.outputs.size(), 1);
    EXPECT_EQ(functype.outputs[0], valtype::i32);
    EXPECT_EQ(module.funcsec.size(), 0);
    EXPECT_EQ(module.codesec.size(), 0);
}

TEST(parser, type_section_with_multiple_functypes)
{
    // type 0 [void] -> [void]
    // type 1 [i32, i64] -> [i32]
    // type 2 [i32] -> []
    const auto section_contents =
        uint8_t{0x03} + functype_void_to_void + functype_i32i64_to_i32 + functype_i32_to_void;
    const auto bin =
        bytes{wasm_prefix} + uint8_t{0x01} + uint8_t(section_contents.size()) + section_contents;
    const auto module = parse(bin);
    ASSERT_EQ(module.typesec.size(), 3);
    const auto functype0 = module.typesec[0];
    EXPECT_EQ(functype0.inputs.size(), 0);
    EXPECT_EQ(functype0.outputs.size(), 0);
    const auto functype1 = module.typesec[1];
    EXPECT_EQ(functype1.inputs.size(), 2);
    EXPECT_EQ(functype1.inputs[0], valtype::i32);
    EXPECT_EQ(functype1.inputs[1], valtype::i64);
    EXPECT_EQ(functype1.outputs.size(), 1);
    EXPECT_EQ(functype1.outputs[0], valtype::i32);
    const auto functype2 = module.typesec[2];
    EXPECT_EQ(functype2.inputs.size(), 1);
    EXPECT_EQ(functype2.inputs[0], valtype::i32);
    EXPECT_EQ(functype2.outputs.size(), 0);
    EXPECT_EQ(module.funcsec.size(), 0);
    EXPECT_EQ(module.codesec.size(), 0);
}

TEST(parser, code_with_empty_expr_2_locals)
{
    // Func with 2x i32 locals, only 0x0b "end" instruction.
    const auto func_2_locals_bin = from_hex("01027f0b");

    const auto code_bin = uint8_t(func_2_locals_bin.size()) + func_2_locals_bin;

    const auto [code_obj, end_pos1] = parser<code>{}(code_bin.data());
    EXPECT_EQ(code_obj.local_count, 2);
    ASSERT_EQ(code_obj.instructions.size(), 1);
    EXPECT_EQ(code_obj.instructions[0], instr::end);
    EXPECT_EQ(code_obj.immediates.size(), 0);
}

TEST(parser, code_with_empty_expr_5_locals)
{
    // Func with 1x i64 + 4x i32 locals , only 0x0b "end" instruction.
    const auto func_5_locals_bin = from_hex("02017f047e0b");

    const auto code_bin = uint8_t(func_5_locals_bin.size()) + func_5_locals_bin;

    const auto [code_obj, end_pos1] = parser<code>{}(code_bin.data());
    EXPECT_EQ(code_obj.local_count, 5);
    ASSERT_EQ(code_obj.instructions.size(), 1);
    EXPECT_EQ(code_obj.instructions[0], instr::end);
    EXPECT_EQ(code_obj.immediates.size(), 0);
}

TEST(parser, code_section_with_2_trivial_codes)
{
    const auto func_nolocals_bin = from_hex("000b");
    const auto code_bin = uint8_t(func_nolocals_bin.size()) + func_nolocals_bin;
    const auto section_contents = uint8_t{2} + code_bin + code_bin;
    const auto bin =
        bytes{wasm_prefix} + uint8_t{10} + uint8_t(section_contents.size()) + section_contents;
    const auto module = parse(bin);
    EXPECT_EQ(module.typesec.size(), 0);
    ASSERT_EQ(module.codesec.size(), 2);
    EXPECT_EQ(module.codesec[0].local_count, 0);
    ASSERT_EQ(module.codesec[0].instructions.size(), 1);
    EXPECT_EQ(module.codesec[0].instructions[0], instr::end);
    EXPECT_EQ(module.codesec[1].local_count, 0);
    ASSERT_EQ(module.codesec[1].instructions.size(), 1);
    EXPECT_EQ(module.codesec[1].instructions[0], instr::end);
}

TEST(parser, code_section_with_basic_instructions)
{
    const auto func_bin = from_hex(
        "00"  // vec(locals)
        "2001210222036a01000b");
    const auto code_bin = uint8_t(func_bin.size()) + func_bin;
    const auto section_contents = uint8_t{1} + code_bin;
    const auto bin =
        bytes{wasm_prefix} + uint8_t{10} + uint8_t(section_contents.size()) + section_contents;
    const auto module = parse(bin);
    EXPECT_EQ(module.typesec.size(), 0);
    ASSERT_EQ(module.codesec.size(), 1);
    EXPECT_EQ(module.codesec[0].local_count, 0);
    ASSERT_EQ(module.codesec[0].instructions.size(), 7);
    EXPECT_EQ(module.codesec[0].instructions[0], instr::local_get);
    EXPECT_EQ(module.codesec[0].instructions[1], instr::local_set);
    EXPECT_EQ(module.codesec[0].instructions[2], instr::local_tee);
    EXPECT_EQ(module.codesec[0].instructions[3], instr::i32_add);
    EXPECT_EQ(module.codesec[0].instructions[4], instr::nop);
    EXPECT_EQ(module.codesec[0].instructions[5], instr::unreachable);
    EXPECT_EQ(module.codesec[0].instructions[6], instr::end);
    ASSERT_EQ(module.codesec[0].immediates.size(), 3 * 4);
    EXPECT_EQ(module.codesec[0].immediates, from_hex("010000000200000003000000"));
}

TEST(parser, milestone1)
{
    /*
    (module
      (func $add (param $lhs i32) (param $rhs i32) (result i32)
        (local $local1 i32)
        local.get $lhs
        local.get $rhs
        i32.add
        local.get $local1
        i32.add
        local.tee $local1
        local.get $lhs
        i32.add
      )
    )
    */

    const auto bin = from_hex(
        "0061736d0100000001070160027f7f017f030201000a13011101017f200020016a20026a220220006a0b");
    const auto m = parse(bin);

    ASSERT_EQ(m.typesec.size(), 1);
    EXPECT_EQ(m.typesec[0].inputs, (std::vector{valtype::i32, valtype::i32}));
    EXPECT_EQ(m.typesec[0].outputs, (std::vector{valtype::i32}));

    ASSERT_EQ(m.codesec.size(), 1);
    const auto& c = m.codesec[0];
    EXPECT_EQ(c.local_count, 1);
    EXPECT_EQ(c.instructions,
        (std::vector{instr::local_get, instr::local_get, instr::i32_add, instr::local_get,
            instr::i32_add, instr::local_tee, instr::local_get, instr::i32_add, instr::end}));
    EXPECT_EQ(c.immediates, from_hex("00000000"
                                     "01000000"
                                     "02000000"
                                     "02000000"
                                     "00000000"));
}