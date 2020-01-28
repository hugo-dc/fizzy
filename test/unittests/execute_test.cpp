#include "execute.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <test/utils/hex.hpp>

using namespace fizzy;

namespace
{
execution_result execute_unary_operation(Instr instr, uint64_t arg)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(Code{0, {Instr::local_get, instr, Instr::end}, {0, 0, 0, 0}});

    return execute(module, 0, {arg});
}

execution_result execute_binary_operation(Instr instr, uint64_t lhs, uint64_t rhs)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, instr, Instr::end}, {0, 0, 0, 0, 1, 0, 0, 0}});

    return execute(module, 0, {lhs, rhs});
}
}  // namespace

TEST(execute, end)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(Code{0, {Instr::end}, {}});

    const auto [trap, ret] = execute(module, 0, {});

    ASSERT_FALSE(trap);
    EXPECT_EQ(ret.size(), 0);
}

TEST(execute, call)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{}, {ValType::i32}});
    module->funcsec.emplace_back(TypeIdx{0});
    module->funcsec.emplace_back(TypeIdx{0});
    module->codesec.emplace_back(Code{0, {Instr::i32_const, Instr::end}, {42, 0, 42, 0}});
    module->codesec.emplace_back(Code{0, {Instr::call, Instr::end}, {0, 0, 0, 0}});

    const auto [trap, ret] = execute(module, 1, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0x2a002a);
}

TEST(execute, call_trap)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{}, {ValType::i32}});
    module->funcsec.emplace_back(TypeIdx{0});
    module->funcsec.emplace_back(TypeIdx{0});
    module->codesec.emplace_back(Code{0, {Instr::unreachable, Instr::end}, {}});
    module->codesec.emplace_back(Code{0, {Instr::call, Instr::end}, {0, 0, 0, 0}});

    const auto [trap, ret] = execute(module, 1, {});

    ASSERT_TRUE(trap);
}

TEST(execute, call_with_arguments)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{ValType::i32, ValType::i32}, {ValType::i32}});
    module->typesec.emplace_back(FuncType{{}, {ValType::i32}});
    module->funcsec.emplace_back(TypeIdx{0});
    module->funcsec.emplace_back(TypeIdx{1});
    module->codesec.emplace_back(Code{2, {Instr::local_get, Instr::end}, {0, 0, 0, 0}});
    module->codesec.emplace_back(
        Code{0, {Instr::i32_const, Instr::i32_const, Instr::call, Instr::end},
            {1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0}});

    const auto [trap, ret] = execute(module, 1, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0x2);
}

TEST(execute, drop)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(
        Code{1, {Instr::local_get, Instr::drop, Instr::end}, {0, 0, 0, 0}});

    const auto [trap, ret] = execute(module, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
}

TEST(execute, select)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::local_get, Instr::select, Instr::end},
            {0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0}});

    auto result = execute(module, 0, {3, 6, 0});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 6);

    result = execute(module, 0, {3, 6, 1});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 3);

    result = execute(module, 0, {3, 6, 42});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 3);
}

TEST(execute, local_get)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(Code{0, {Instr::local_get, Instr::end}, {0, 0, 0, 0}});

    const auto [trap, ret] = execute(module, 0, {42});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, local_set)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(
        Code{1, {Instr::local_get, Instr::local_set, Instr::local_get, Instr::end},
            {0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}});

    const auto [trap, ret] = execute(module, 0, {42});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, local_tee)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(
        Code{1, {Instr::local_get, Instr::local_tee, Instr::end}, {0, 0, 0, 0, 1, 0, 0, 0}});

    const auto [trap, ret] = execute(module, 0, {42});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, global_get)
{
    auto module = std::make_shared<Module>();
    module->globalsec.emplace_back(Global{true, {ConstantExpression::Kind::Constant, {42}}});

    module->codesec.emplace_back(Code{0, {Instr::global_get, Instr::end}, {0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));

    const auto [trap, ret] = execute(instance, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, global_get_two_globals)
{
    auto module = std::make_shared<Module>();
    module->globalsec.emplace_back(Global{true, {ConstantExpression::Kind::Constant, {42}}});
    module->globalsec.emplace_back(Global{true, {ConstantExpression::Kind::Constant, {43}}});

    module->codesec.emplace_back(Code{0, {Instr::global_get, Instr::end}, {0, 0, 0, 0}});
    module->codesec.emplace_back(Code{0, {Instr::global_get, Instr::end}, {1, 0, 0, 0}});

    auto instance = instantiate(std::move(module));

    auto [trap, ret] = execute(instance, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);

    auto [trap2, ret2] = execute(instance, 1, {});

    ASSERT_FALSE(trap2);
    ASSERT_EQ(ret2.size(), 1);
    EXPECT_EQ(ret2[0], 43);
}

TEST(execute, global_get_imported)
{
    auto module = std::make_shared<Module>();
    module->importsec.emplace_back(Import{"mod", "glob", ExternalKind::Global, {false}});
    module->codesec.emplace_back(Code{0, {Instr::global_get, Instr::end}, {0, 0, 0, 0}});

    uint64_t global_value = 42;
    auto instance = instantiate(std::move(module), {}, {ImportedGlobal{&global_value, false}});

    const auto [trap, ret] = execute(instance, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);

    global_value = 43;

    const auto [trap2, ret2] = execute(instance, 0, {});

    ASSERT_FALSE(trap2);
    ASSERT_EQ(ret2.size(), 1);
    EXPECT_EQ(ret2[0], 43);
}

TEST(execute, global_set)
{
    auto module = std::make_shared<Module>();
    module->globalsec.emplace_back(Global{true, {ConstantExpression::Kind::Constant, {41}}});

    module->codesec.emplace_back(
        Code{0, {Instr::i32_const, Instr::global_set, Instr::end}, {42, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));

    const auto [trap, ret] = execute(instance, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(instance.globals[0], 42);
}

TEST(execute, global_set_two_globals)
{
    auto module = std::make_shared<Module>();
    module->globalsec.emplace_back(Global{true, {ConstantExpression::Kind::Constant, {42}}});
    module->globalsec.emplace_back(Global{true, {ConstantExpression::Kind::Constant, {43}}});

    module->codesec.emplace_back(Code{0,
        {Instr::i32_const, Instr::global_set, Instr::i32_const, Instr::global_set, Instr::end},
        {44, 0, 0, 0, 0, 0, 0, 0, 45, 0, 0, 0, 1, 0, 0, 0}});

    auto instance = instantiate(std::move(module));

    const auto [trap, ret] = execute(instance, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(instance.globals[0], 44);
    ASSERT_EQ(instance.globals[1], 45);
}

TEST(execute, global_set_imported)
{
    auto module = std::make_shared<Module>();
    module->importsec.emplace_back(Import{"mod", "glob", ExternalKind::Global, {true}});
    module->codesec.emplace_back(
        Code{0, {Instr::i32_const, Instr::global_set, Instr::end}, {42, 0, 0, 0, 0, 0, 0, 0}});

    uint64_t global_value = 41;
    auto instance = instantiate(std::move(module), {}, {ImportedGlobal{&global_value, true}});

    const auto [trap, ret] = execute(instance, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(global_value, 42);
}

TEST(execute, i32_const)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(Code{0, {Instr::i32_const, Instr::end}, {0x42, 0, 0x42, 0}});

    const auto [trap, ret] = execute(module, 0, {});

    ASSERT_EQ(trap, false);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0x420042);
}

TEST(execute, i64_const)
{
    auto module = std::make_shared<Module>();
    module->codesec.emplace_back(
        Code{0, {Instr::i64_const, Instr::end}, {0x42, 0, 0x42, 0, 0, 0, 0, 1}});

    const auto [trap, ret] = execute(module, 0, {});

    ASSERT_EQ(trap, false);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0x0100000000420042ULL);
}

TEST(execute, i32_load)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i32_load, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 42;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], 42);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i64_load)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i64_load, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x2a;
    instance.memory[4] = 0x2a;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], 0x2a0000002a);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i32_load8_s)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i32_load8_s, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x80;
    instance.memory[1] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(static_cast<int32_t>(ret[0]), -128);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i32_load8_u)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i32_load8_u, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x81;
    instance.memory[1] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0x01);
    ASSERT_EQ(static_cast<uint32_t>(ret[0]), 129);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i32_load16_s)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i32_load16_s, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x00;
    instance.memory[1] = 0x80;
    instance.memory[3] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(static_cast<int32_t>(ret[0]), -32768);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i32_load16_u)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i32_load16_u, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x01;
    instance.memory[1] = 0x80;
    instance.memory[3] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0x01);
    ASSERT_EQ(static_cast<uint32_t>(ret[0]), 32769);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i64_load8_s)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i64_load8_s, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x80;
    instance.memory[1] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], uint64_t(-128));

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i64_load8_u)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i64_load8_u, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x81;
    instance.memory[1] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], 0x81);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i64_load16_s)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i64_load16_s, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x00;
    instance.memory[1] = 0x80;
    instance.memory[2] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], uint64_t(-32768));

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i64_load16_u)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i64_load16_u, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x01;
    instance.memory[1] = 0x80;
    instance.memory[2] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], 0x8001);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i64_load32_s)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i64_load32_s, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x00;
    instance.memory[1] = 0x00;
    instance.memory[2] = 0x00;
    instance.memory[3] = 0x80;
    instance.memory[4] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], uint64_t(-2147483648));

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i64_load32_u)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::i64_load32_u, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    instance.memory[0] = 0x01;
    instance.memory[1] = 0x00;
    instance.memory[2] = 0x00;
    instance.memory[3] = 0x80;
    instance.memory[4] = 0xf1;
    const auto [trap, ret] = execute(instance, 0, {0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    ASSERT_EQ(ret[0], 0x80000001);

    ASSERT_TRUE(execute(instance, 0, {65537}).trapped);
}

TEST(execute, i32_store)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i32_store, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {42, 0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.substr(0, 4), from_hex("2a000000"));

    ASSERT_TRUE(execute(instance, 0, {42, 65537}).trapped);
}

TEST(execute, i64_store)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i64_store, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {0x2a0000002a, 0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.substr(0, 8), from_hex("2a0000002a000000"));

    ASSERT_TRUE(execute(instance, 0, {0x2a0000002a, 65537}).trapped);
}

TEST(execute, i32_store8)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i32_store8, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {0xf1f2f380, 0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.substr(0, 4), from_hex("80000000"));

    ASSERT_TRUE(execute(instance, 0, {0xf1f2f380, 65537}).trapped);
}

TEST(execute, i32_store8_trap)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i32_store8, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {0xf1f2f380, 65537});

    ASSERT_TRUE(trap);
}

TEST(execute, i32_store16)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i32_store16, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {0xf1f28000, 0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.substr(0, 4), from_hex("00800000"));

    ASSERT_TRUE(execute(instance, 0, {0xf1f28000, 65537}).trapped);
}

TEST(execute, i64_store8)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i64_store8, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {0xf1f2f4f5f6f7f880, 0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.substr(0, 8), from_hex("8000000000000000"));

    ASSERT_TRUE(execute(instance, 0, {0xf1f2f4f5f6f7f880, 65537}).trapped);
}

TEST(execute, i64_store16)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i64_store16, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {0xf1f2f4f5f6f78000, 0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.substr(0, 8), from_hex("0080000000000000"));

    ASSERT_TRUE(execute(instance, 0, {0xf1f2f4f5f6f78000, 65537}).trapped);
}

TEST(execute, i64_store32)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::local_get, Instr::i64_store32, Instr::end},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    const auto [trap, ret] = execute(instance, 0, {0xf1f2f4f580000000, 0});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.substr(0, 8), from_hex("0000008000000000"));

    ASSERT_TRUE(execute(instance, 0, {0xf1f2f4f580000000, 65537}).trapped);
}

TEST(execute, memory_size)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(Code{0, {Instr::memory_size, Instr::end}, {}});

    const auto [trap, ret] = execute(module, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
}

TEST(execute, memory_grow)
{
    auto module = std::make_shared<Module>();
    module->memorysec.emplace_back(Memory{{1, 4096}});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::memory_grow, Instr::end}, {0, 0, 0, 0}});

    auto result = execute(module, 0, {0});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute(module, 0, {1});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    // 256MB memory.
    result = execute(module, 0, {4095});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    // >256MB memory.
    result = execute(module, 0, {4096});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], uint32_t(-1));

    // Way too high (but still within bounds)
    result = execute(module, 0, {0xffffffe});

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], uint32_t(-1));
}

TEST(execute, i32_eqz)
{
    auto result = execute_unary_operation(Instr::i32_eqz, 0);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_unary_operation(Instr::i32_eqz, 1);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    // Dirty stack
    result = execute_unary_operation(fizzy::Instr::i32_eqz, 0xff00000000);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_unary_operation(fizzy::Instr::i32_eqz, 0xff00000001);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i32_eq)
{
    auto result = execute_binary_operation(Instr::i32_eq, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_eq, 22, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i32_ne)
{
    auto result = execute_binary_operation(Instr::i32_ne, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_ne, 22, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i32_lt_s)
{
    auto result = execute_binary_operation(Instr::i32_lt_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_lt_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_lt_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_lt_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i32_lt_u)
{
    auto result = execute_binary_operation(Instr::i32_lt_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_lt_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i32_gt_s)
{
    auto result = execute_binary_operation(Instr::i32_gt_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_gt_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_gt_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_gt_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i32_gt_u)
{
    auto result = execute_binary_operation(Instr::i32_gt_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_gt_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i32_le_s)
{
    auto result = execute_binary_operation(Instr::i32_le_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_le_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_le_s, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_le_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_le_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_le_s, uint64_t(-42), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i32_le_u)
{
    auto result = execute_binary_operation(Instr::i32_le_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_le_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_le_u, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i32_ge_s)
{
    auto result = execute_binary_operation(Instr::i32_ge_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_ge_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_ge_s, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_ge_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_ge_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_ge_s, uint64_t(-42), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i32_ge_u)
{
    auto result = execute_binary_operation(Instr::i32_ge_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i32_ge_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i32_ge_u, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i64_eqz)
{
    auto result = execute_unary_operation(Instr::i64_eqz, 0);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_unary_operation(Instr::i64_eqz, 1);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    // 64-bit value on the stack
    result = execute_unary_operation(fizzy::Instr::i64_eqz, 0xff00000000);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_unary_operation(fizzy::Instr::i64_eqz, 0xff00000001);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i64_eq)
{
    auto result = execute_binary_operation(Instr::i64_eq, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_eq, 22, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i64_ne)
{
    auto result = execute_binary_operation(Instr::i64_ne, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_ne, 22, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i64_lt_s)
{
    auto result = execute_binary_operation(Instr::i64_lt_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_lt_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_lt_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_lt_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i64_lt_u)
{
    auto result = execute_binary_operation(Instr::i64_lt_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_lt_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i64_gt_s)
{
    auto result = execute_binary_operation(Instr::i64_gt_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_gt_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_gt_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_gt_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i64_gt_u)
{
    auto result = execute_binary_operation(Instr::i64_gt_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_gt_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);
}

TEST(execute, i64_le_s)
{
    auto result = execute_binary_operation(Instr::i64_le_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_le_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_le_s, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_le_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_le_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_le_s, uint64_t(-42), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i64_le_u)
{
    auto result = execute_binary_operation(Instr::i64_le_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_le_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_le_u, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i64_ge_s)
{
    auto result = execute_binary_operation(Instr::i64_ge_s, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_ge_s, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_ge_s, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_ge_s, uint64_t(-41), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_ge_s, uint64_t(-42), uint64_t(-41));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_ge_s, uint64_t(-42), uint64_t(-42));

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i64_ge_u)
{
    auto result = execute_binary_operation(Instr::i64_ge_u, 22, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);

    result = execute_binary_operation(Instr::i64_ge_u, 20, 22);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 0);

    result = execute_binary_operation(Instr::i64_ge_u, 20, 20);

    ASSERT_FALSE(result.trapped);
    ASSERT_EQ(result.stack.size(), 1);
    EXPECT_EQ(result.stack[0], 1);
}

TEST(execute, i32_clz)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i32_clz, 0x7f);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 32 - 7);
}

TEST(execute, i32_clz0)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i32_clz, 0);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 32);
}

TEST(execute, i32_ctz)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i32_ctz, 0x80);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 7);
}

TEST(execute, i32_ctz0)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i32_ctz, 0);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 32);
}

TEST(execute, i32_popcnt)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i32_popcnt, 0x7fff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 7 + 8);
}

TEST(execute, i32_add)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_add, 22, 20);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i32_sub)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_sub, 424242, 424200);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i32_mul)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_mul, 2, 21);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i32_div_s)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_div_s, uint64_t(-84), 2);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], uint64_t(-42));
}

TEST(execute, i32_div_s_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_div_s, 84, 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i32_div_s_overflow)
{
    const auto [trap, ret] = execute_binary_operation(
        Instr::i32_div_s, uint64_t(std::numeric_limits<int32_t>::min()), uint64_t(-1));

    ASSERT_TRUE(trap);
}

TEST(execute, i32_div_u)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_div_u, 84, 2);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i32_div_u_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_div_u, 84, 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i32_rem_s)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_rem_s, uint64_t(-4242), 4200);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], uint64_t(-42));
}

TEST(execute, i32_rem_s_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_rem_s, uint64_t(-4242), 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i32_rem_u)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_rem_u, 4242, 4200);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i32_rem_u_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_rem_u, 4242, 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i32_and)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_and, 0x00ffff, 0xffff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xff00);
}

TEST(execute, i32_or)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_or, 0x00ffff, 0xffff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xffffff);
}

TEST(execute, i32_xor)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_xor, 0x00ffff, 0xffff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xff00ff);
}

TEST(execute, i32_shl)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_shl, 21, 1);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i32_shr_s)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_shr_s, uint64_t(-84), 1);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], uint64_t(-42));
}

TEST(execute, i32_shr_u)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_shr_u, 84, 1);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i32_rotl)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_rotl, 0xff000000, 4);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xf000000f);
}

TEST(execute, i32_rotr)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i32_rotr, 0x000000ff, 4);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xf000000f);
}

TEST(execute, i32_wrap_i64)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i32_wrap_i64, 0xffffffffffffffff);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xffffffff);
}

TEST(execute, i64_extend_i32_s_all_bits_set)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_extend_i32_s, 0xffffffff);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xffffffffffffffff);
}

TEST(execute, i64_extend_i32_s_one_bit_set)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_extend_i32_s, 0x80000000);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xffffffff80000000);
}

TEST(execute, i64_extend_i32_s_0)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_extend_i32_s, 0);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0);
}

TEST(execute, i64_extend_i32_s_1)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_extend_i32_s, 0x01);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0x01);
}

TEST(execute, i64_extend_i32_u)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_extend_i32_u, 0xff000000);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0x00000000ff000000);
}

TEST(execute, i64_clz)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_clz, 0x7f);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 64 - 7);
}

TEST(execute, i64_clz0)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_clz, 0);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 64);
}

TEST(execute, i64_ctz)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_ctz, 0x80);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 7);
}

TEST(execute, i64_ctz0)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_ctz, 0);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 64);
}

TEST(execute, i64_popcnt)
{
    const auto [trap, ret] = execute_unary_operation(Instr::i64_popcnt, 0x7fff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 7 + 8);
}

TEST(execute, i64_add)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_add, 22, 20);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i64_sub)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_sub, 424242, 424200);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i64_mul)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_mul, 2, 21);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i64_div_s)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_div_s, uint64_t(-84), 2);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], uint64_t(-42));
}

TEST(execute, i64_div_s_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_div_s, 84, 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i64_div_s_overflow)
{
    const auto [trap, ret] = execute_binary_operation(
        Instr::i64_div_s, uint64_t(std::numeric_limits<int64_t>::min()), uint64_t(-1));

    ASSERT_TRUE(trap);
}

TEST(execute, i64_div_u)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_div_u, 84, 2);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i64_div_u_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_div_u, 84, 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i64_rem_s)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_rem_s, uint64_t(-4242), 4200);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], uint64_t(-42));
}

TEST(execute, i64_rem_s_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_rem_s, uint64_t(-4242), 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i64_rem_u)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_rem_u, 4242, 4200);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i64_rem_u_by_zero)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_rem_u, 4242, 0);

    ASSERT_TRUE(trap);
}

TEST(execute, i64_and)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_and, 0x00ffff, 0xffff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xff00);
}

TEST(execute, i64_or)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_or, 0x00ffff, 0xffff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xffffff);
}

TEST(execute, i64_xor)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_xor, 0x00ffff, 0xffff00);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xff00ff);
}

TEST(execute, i64_shl)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_shl, 21, 1);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i64_shr_s)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_shr_s, uint64_t(-84), 1);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], uint64_t(-42));
}

TEST(execute, i64_shr_u)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_shr_u, 84, 1);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, i64_rotl)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_rotl, 0xff00000000000000, 4);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xf00000000000000f);
}

TEST(execute, i64_rotr)
{
    const auto [trap, ret] = execute_binary_operation(Instr::i64_rotr, 0xff, 4);

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 0xf00000000000000f);
}

TEST(execute, start_section)
{
    // In this test the start function (index 1) writes a i32 value to the memory
    // and the same is read back in the "main" function (index 0).
    auto module = std::make_shared<Module>();
    module->startfunc = 1;
    module->memorysec.emplace_back(Memory{{1, 1}});
    module->codesec.emplace_back(
        Code{0, {Instr::i32_const, Instr::i32_load, Instr::end}, {0, 0, 0, 0, 0, 0, 0, 0}});
    module->codesec.emplace_back(
        Code{0, {Instr::i32_const, Instr::i32_const, Instr::i32_store, Instr::end},
            {0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0}});

    auto instance = instantiate(std::move(module));
    // Start function sets this
    ASSERT_EQ(instance.memory.substr(0, 4), from_hex("2a000000"));

    const auto [trap, ret] = execute(instance, 0, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
    EXPECT_EQ(instance.memory.substr(0, 4), from_hex("2a000000"));
}

TEST(execute, imported_function)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{ValType::i32, ValType::i32}, {ValType::i32}});
    module->importsec.emplace_back(Import{"mod", "foo", ExternalKind::Function, {0}});

    auto host_foo = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] + args[1]}};
    };

    auto instance = instantiate(std::move(module), {host_foo});

    const auto [trap, ret] = execute(instance, 0, {20, 22});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, imported_two_functions)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{ValType::i32, ValType::i32}, {ValType::i32}});
    module->importsec.emplace_back(Import{"mod", "foo1", ExternalKind::Function, {0}});
    module->importsec.emplace_back(Import{"mod", "foo2", ExternalKind::Function, {0}});

    auto host_foo1 = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] + args[1]}};
    };
    auto host_foo2 = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] * args[1]}};
    };

    auto instance = instantiate(std::move(module), {host_foo1, host_foo2});

    const auto [trap1, ret1] = execute(instance, 0, {20, 22});

    ASSERT_FALSE(trap1);
    ASSERT_EQ(ret1.size(), 1);
    EXPECT_EQ(ret1[0], 42);

    const auto [trap2, ret2] = execute(instance, 1, {20, 22});

    ASSERT_FALSE(trap2);
    ASSERT_EQ(ret2.size(), 1);
    EXPECT_EQ(ret2[0], 440);
}

TEST(execute, imported_functions_and_regular_one)
{
    const auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{ValType::i32, ValType::i32}, {ValType::i32}});
    module->typesec.emplace_back(FuncType{{ValType::i64}, {ValType::i64}});
    module->importsec.emplace_back(Import{"mod", "foo1", ExternalKind::Function, {0}});
    module->importsec.emplace_back(Import{"mod", "foo2", ExternalKind::Function, {0}});
    module->codesec.emplace_back(Code{0, {Instr::i32_const, Instr::end}, {42, 0, 42, 0}});

    auto host_foo1 = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] + args[1]}};
    };
    auto host_foo2 = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] * args[0]}};
    };

    auto instance = instantiate(module, {host_foo1, host_foo2});

    const auto [trap1, ret1] = execute(instance, 0, {20, 22});

    ASSERT_FALSE(trap1);
    ASSERT_EQ(ret1.size(), 1);
    EXPECT_EQ(ret1[0], 42);

    const auto [trap2, ret2] = execute(instance, 1, {20});

    ASSERT_FALSE(trap2);
    ASSERT_EQ(ret2.size(), 1);
    EXPECT_EQ(ret2[0], 400);

    // check correct number of arguments is passed to host
    auto count_args = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args.size()}};
    };

    auto instance_couner = instantiate(module, {count_args, count_args});

    const auto [trap3, ret3] = execute(instance_couner, 0, {20, 22});

    ASSERT_FALSE(trap3);
    ASSERT_EQ(ret3.size(), 1);
    EXPECT_EQ(ret3[0], 2);

    const auto [trap4, ret4] = execute(instance_couner, 1, {20});

    ASSERT_FALSE(trap4);
    ASSERT_EQ(ret4.size(), 1);
    EXPECT_EQ(ret4[0], 1);
}

TEST(execute, imported_two_functions_different_type)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{ValType::i32, ValType::i32}, {ValType::i32}});
    module->typesec.emplace_back(FuncType{{ValType::i64}, {ValType::i64}});
    module->importsec.emplace_back(Import{"mod", "foo1", ExternalKind::Function, {0}});
    module->importsec.emplace_back(Import{"mod", "foo2", ExternalKind::Function, {0}});
    module->codesec.emplace_back(Code{0, {Instr::i32_const, Instr::end}, {42, 0, 42, 0}});

    auto host_foo1 = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] + args[1]}};
    };
    auto host_foo2 = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] * args[0]}};
    };

    auto instance = instantiate(std::move(module), {host_foo1, host_foo2});

    const auto [trap1, ret1] = execute(instance, 0, {20, 22});

    ASSERT_FALSE(trap1);
    ASSERT_EQ(ret1.size(), 1);
    EXPECT_EQ(ret1[0], 42);

    const auto [trap2, ret2] = execute(instance, 1, {20});

    ASSERT_FALSE(trap2);
    ASSERT_EQ(ret2.size(), 1);
    EXPECT_EQ(ret2[0], 400);

    const auto [trap3, ret3] = execute(instance, 2, {20});

    ASSERT_FALSE(trap3);
    ASSERT_EQ(ret3.size(), 1);
    EXPECT_EQ(ret3[0], 0x2a002a);
}

TEST(execute, imported_function_traps)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{ValType::i32, ValType::i32}, {ValType::i32}});
    module->importsec.emplace_back(Import{"mod", "foo", ExternalKind::Function, {0}});

    auto host_foo = [](Instance&, std::vector<uint64_t>) -> execution_result { return {true, {}}; };

    auto instance = instantiate(std::move(module), {host_foo});

    const auto [trap, ret] = execute(instance, 0, {20, 22});

    ASSERT_TRUE(trap);
}

TEST(execute, imported_function_call)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{}, {ValType::i32}});
    module->importsec.emplace_back(Import{"mod", "foo", ExternalKind::Function, {0}});
    module->funcsec.emplace_back(TypeIdx{0});
    module->codesec.emplace_back(Code{0, {Instr::call, Instr::end}, {0, 0, 0, 0}});

    auto host_foo = [](Instance&, std::vector<uint64_t>) -> execution_result {
        return {false, {42}};
    };

    auto instance = instantiate(std::move(module), {host_foo});

    const auto [trap, ret] = execute(instance, 1, {});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, imported_function_call_with_arguments)
{
    auto module = std::make_shared<Module>();
    module->typesec.emplace_back(FuncType{{ValType::i32}, {ValType::i32}});
    module->importsec.emplace_back(Import{"mod", "foo", ExternalKind::Function, {0}});
    module->funcsec.emplace_back(TypeIdx{0});
    module->codesec.emplace_back(
        Code{0, {Instr::local_get, Instr::call, Instr::i32_const, Instr::i32_add, Instr::end},
            {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0}});

    auto host_foo = [](Instance&, std::vector<uint64_t> args) -> execution_result {
        return {false, {args[0] * 2}};
    };

    auto instance = instantiate(std::move(module), {host_foo});

    const auto [trap, ret] = execute(instance, 1, {20});

    ASSERT_FALSE(trap);
    ASSERT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], 42);
}

TEST(execute, memory_copy_32bytes)
{
    /* copy32(dst, src) - copies 4 x 8 bytes using offset immediate.
    (memory 1)
    (func (param i32 i32)
      get_local 0
      get_local 1
      i64.load offset=0
      i64.store offset=0
      get_local 0
      get_local 1
      i64.load offset=8
      i64.store offset=8
      get_local 0
      get_local 1
      i64.load offset=16
      i64.store offset=16
      get_local 0
      get_local 1
      i64.load offset=24
      i64.store offset=24
    )
    */

    const auto bin = from_hex(
        "0061736d0100000001060160027f7f000302010005030100010a2c012a00200020012903003703002000200129"
        "030837030820002001290310370310200020012903183703180b000e046e616d65020701000200000100");

    auto instance = instantiate(parse(bin));
    ASSERT_EQ(instance.memory.size(), 65536);
    const auto input = from_hex("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");
    ASSERT_EQ(input.size(), 32);
    std::copy(input.begin(), input.end(), instance.memory.begin());
    const auto [trap, ret] = execute(instance, 0, {33, 0});
    ASSERT_FALSE(trap);
    EXPECT_EQ(ret.size(), 0);
    ASSERT_EQ(instance.memory.size(), 65536);
    bytes output;
    std::copy_n(&instance.memory[33], input.size(), std::back_inserter(output));
    EXPECT_EQ(output, input);
}
