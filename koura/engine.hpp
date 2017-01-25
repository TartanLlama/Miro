#ifndef KOURA_ENGINE_HPP
#define KOURA_ENGINE_HPP

//C++
#include <iostream>
#include <optional>
#include <cctype>
#include <functional>
#include <cassert>

//Koura
#include "context.hpp"
#include "filters.hpp"

namespace koura {
    class engine;

    namespace detail {
        inline void stream_up_to_tag (std::istream& in, std::ostream& out) {
            char c;
            while (c = in.get(), in) {
                if (c == '{') {
                    auto next = in.peek();
                    if (next == '{' || next == '%') {
                        in.unget();
                        return;
                    }
                }
                out << c;
            }
        }

        inline void eat_whitespace (std::istream& in) {
            while (std::isspace(in.peek())) {
                in.get();
            }
        }

        inline char peek (std::istream& in) {
            eat_whitespace(in);
            return in.peek();
        }

        inline std::string get_identifier (std::istream& in) {
            eat_whitespace(in);
            std::string name = "";
            while (std::isalpha(in.peek()) || in.peek() == '_') {
                name += in.get();
            }

            return name;
        }

        inline entity& parse_named_entity (std::istream& in, context& ctx) {
            auto name = get_identifier(in);

            auto& ent = ctx.get_entity(std::string{name});

            eat_whitespace(in);

            if (ent.get_type() == entity::type::text) {
                return ent;
            }
            else if (ent.get_type() == entity::type::object) {
                auto next = in.peek();

                if (next == '.') {
                    if (ent.get_type() != entity::type::object) {
                        //TODO error
                    }

                    auto field_name = get_identifier(in);
                    auto ent_obj = ent.get_value<object_t>();

                    if (!ent_obj.count(field_name)) {
                        //TODO error
                    }

                    return {ent_obj.at(field_name)};
                }
            }
            else if (ent.get_type() == entity::type::sequence) {
                return ent;
            }
        }

        inline koura::entity parse_entity (std::istream& in, koura::context& ctx) {
            auto c = peek(in);
            //String literal
            if (c == '\'') {
                in.get();
                koura::text_t text;
                while (in.peek() != '\'') {
                    text += in.get();
                }
                in.get();
                return entity{text};
            }
            //Number literal
            else if (std::isdigit(c)) {
                koura::number_t num;
                in >> num;
                return entity{num};
            }
            //List literal
            else if (c == '[') {
                //TODO
            }
            //Named entity
            else {
                return parse_named_entity(in,ctx);
            }

            //TODO error
        }


        inline void expect_text(std::istream& in, std::string_view expected) {
            std::string got;
            in >> got;
            if (got != expected) {
                //TODO error
            }
        }

        inline void eat_tag (std::istream& in) {
            eat_whitespace(in);

            if (in.get() != '{') {
                //TODO error
            }

            if (in.get() != '%') {
                //TODO error
            }

            while (in.get() != '%') {
            }

            if (in.get() != '}') {
                //TODO error
            }
        }


        inline bool is_next_tag (std::istream& in, std::string_view tag) {
            auto start_pos = in.tellg();
            auto reset = [start_pos,&in] {in.seekg(start_pos);};

            if (in.get() != '{') {
                reset();
                return false;
            }

            if (in.get() != '%') {
                reset();
                return false;
            }

            while (std::isspace(in.peek())) {
                in.get();
            }

            auto id = get_identifier(in);

            if (id == tag) {
                reset();
                return true;
            }

            reset();
            return false;
        }

        void process_tag (engine& eng, std::istream& in, std::ostream& out, context& ctx);

        inline void process_until_tag (engine& eng, std::istream& in, std::ostream& out, context& ctx, std::string_view tag) {
            while (true) {
                stream_up_to_tag(in,out);
                if (is_next_tag(in,tag)) {
                    return;
                }
                process_tag(eng,in,out,ctx);
            }
        }

        inline void handle_for_expression (engine& eng, std::istream& in, std::ostream& out, context& ctx) {
            auto loop_var_id = get_identifier(in);
            expect_text(in, "in");
            auto ent = parse_entity(in,ctx);

            if (in.get() != '%' || in.get() != '}') {
                //TODO error
            }

            auto start_pos = in.tellg();

            if (ent.get_type() != entity::type::sequence) {
                //TODO error
            }

            auto seq = ent.get_value<sequence_t>();

            for (auto&& loop_var : seq) {
                in.seekg(start_pos);
                auto ctx_with_loop_var = ctx;
                ctx_with_loop_var.add_entity(loop_var_id, loop_var);

                process_until_tag(eng,in,out,ctx_with_loop_var,"endfor");
            }

            eat_tag(in);
        }

        inline void handle_unless_expression (engine& eng, std::istream& in, std::ostream& out, context& ctx) {

        }

        inline void handle_set_expression (engine& eng, std::istream& in, std::ostream& out, context& ctx) {
            auto& ent = parse_named_entity(in, ctx);
            auto val = parse_entity(in, ctx);

            switch (ent.get_type()) {
            case koura::entity::type::number:
                ent.get_value<koura::number_t>() = val.get_value<koura::number_t>();
                break;
            case koura::entity::type::text:
            {
                ent.get_value<koura::text_t>() = val.get_value<koura::text_t>();
                auto a = ent.get_value<koura::text_t>();
                auto b = val.get_value<koura::text_t>();
                break;
            }
            case koura::entity::type::object:
                ent.get_value<koura::object_t>() = val.get_value<koura::object_t>();
                break;
            case koura::entity::type::sequence:
                //TODO
                break;
            }

            eat_whitespace(in);
            assert(in.get() == '%');
            assert(in.get() == '}');
        }

        inline void handle_if_expression (engine& eng, std::istream& in, std::ostream& out, context& ctx) {
            auto ent = parse_entity(in, ctx);


        }
    }

    class engine {
    public:
        using expression_handler_t = std::function<void(engine&,std::istream&, std::ostream&, context&)>;
        using filter_t = std::function<std::string(std::string_view, context&)>;

        engine() :
            m_expression_handlers{
              {"if", detail::handle_if_expression},
              {"unless", detail::handle_unless_expression},
              {"set", detail::handle_set_expression},
              {"for", detail::handle_for_expression}
            },

            m_filters{
              {"capitalize", filters::capitalize}
            }
        {}

        void render (std::istream& in, std::ostream& out, context& ctx) {
            while (in) {
                detail::stream_up_to_tag(in, out);
                if (in) {
                    detail::process_tag(*this, in, out, ctx);
                }
            }
        }

        void register_custom_expression (std::string_view name, expression_handler_t handler) {
            m_expression_handlers.emplace(std::string{name}, handler);
        }

        void register_custom_filter (std::string_view name, filter_t filter) {
            m_filters.emplace(std::string{name}, filter);
        }



        inline void handle_variable_tag (std::istream& in, std::ostream& out, context& ctx) {
            auto ent = detail::parse_named_entity(in, ctx);
            if (ent.get_type() != entity::type::text) {
                //TODO error
            }

            auto text = ent.get_value<text_t>();

            detail::eat_whitespace(in);

            while (in.peek() == '|') {
                in.get();
                text = handle_filter(in, ctx, text);
                detail::eat_whitespace(in);
            }

            out << text;

            assert(in.get() == '}');
            assert(in.get() == '}');
        }

        void handle_expression_tag (std::istream& in, std::ostream& out, context& ctx) {
            std::string tag_name;
            in >> tag_name;

            if (!m_expression_handlers.count(tag_name)) {
                //TODO error
            }

            m_expression_handlers[tag_name](*this,in,out,ctx);
        }

        auto handle_filter (std::istream& in, context& ctx, std::string_view text) -> std::string {
            auto filter_name = detail::get_identifier(in);

            if (!m_filters.count(filter_name)) {
                //TODO error
            }

            return m_filters[filter_name](text,ctx);
        }

    private:
        std::unordered_map<std::string, expression_handler_t> m_expression_handlers;
        std::unordered_map<std::string, filter_t> m_filters;
    };

    namespace detail {
        inline void process_tag (engine& eng, std::istream& in, std::ostream& out, context& ctx) {
            assert(in.get() == '{');

            auto next = in.get();

            if (next == '{') {
                eat_whitespace(in);
                eng.handle_variable_tag(in,out,ctx);
            }

            if (next == '%') {
                eat_whitespace(in);
                eng.handle_expression_tag(in,out,ctx);
                //Get rid of one trailing whitespace
                if (in.peek() == '\n') in.get();
            }
        }
    }
}

#endif
