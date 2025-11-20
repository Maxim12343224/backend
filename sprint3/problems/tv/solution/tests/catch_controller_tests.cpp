#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "../src/controller.h"

SCENARIO("Controller", "[Controller]") {
    using namespace std::literals;
    GIVEN("Controller and TV") {
        TV tv;
        std::istringstream input;
        std::ostringstream output;
        Menu menu{input, output};
        Controller controller{tv, menu};

        auto run_menu_command = [&menu, &input](std::string command) {
            input.str(std::move(command));
            input.clear();
            menu.Run();
        };
        auto expect_output = [&output](std::string_view expected) {
            CHECK(output.str() == std::string{expected});
        };
        auto expect_extra_arguments_error = [&expect_output](std::string_view command) {
            expect_output("Error: the "s.append(command).append(
                " command does not require any arguments\n"sv));
        };
        auto expect_empty_output = [&expect_output] {
            expect_output({});
        };

        WHEN("The TV is turned off") {
            AND_WHEN("Info command is entered without arguments") {
                run_menu_command("Info"s);

                THEN("output contains info that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("Info command is entered with some arguments") {
                run_menu_command("Info some extra arguments");

                THEN("Error message is printed") {
                    expect_extra_arguments_error("Info"s);
                }
            }

            AND_WHEN("Info command has trailing spaces") {
                run_menu_command("Info  "s);

                THEN("output contains information that TV is turned off") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("TurnOn command is entered without arguments") {
                run_menu_command("TurnOn"s);

                THEN("TV is turned on") {
                    CHECK(tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOn command is entered with some arguments") {
                run_menu_command("TurnOn some args"s);

                THEN("the error message is printed and TV is not turned on") {
                    CHECK(!tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOn"s);
                }
            }

            AND_WHEN("SelectChannel command is entered when TV is off") {
                run_menu_command("SelectChannel 5"s);

                THEN("error message is printed") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered when TV is off") {
                run_menu_command("SelectPreviousChannel"s);

                THEN("error message is printed") {
                    expect_output("TV is turned off\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered without arguments") {
                run_menu_command("SelectChannel"s);

                THEN("error message is printed") {
                    expect_output("Error: channel number is required\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with invalid channel") {
                run_menu_command("SelectChannel abc"s);

                THEN("error message is printed") {
                    expect_output("Invalid channel\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with out of range channel") {
                run_menu_command("SelectChannel 0"s);

                THEN("error message is printed") {
                    expect_output("Channel is out of range\n"s);
                }
            }
        }

        WHEN("The TV is turned on") {
            tv.TurnOn();
            
            AND_WHEN("TurnOff command is entered without arguments") {
                run_menu_command("TurnOff"s);

                THEN("TV is turned off") {
                    CHECK(!tv.IsTurnedOn());
                    expect_empty_output();
                }
            }

            AND_WHEN("TurnOff command is entered with some arguments") {
                run_menu_command("TurnOff some args");

                THEN("the error message is printed and TV is not turned off") {
                    CHECK(tv.IsTurnedOn());
                    expect_extra_arguments_error("TurnOff"s);
                }
            }

            AND_WHEN("Info command is entered without arguments") {
                tv.SelectChannel(12);
                run_menu_command("Info"s);

                THEN("current channel is printed") {
                    expect_output("TV is turned on\nChannel number is 12\n"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with valid channel") {
                run_menu_command("SelectChannel 25"s);

                THEN("channel is changed") {
                    CHECK(tv.GetChannel() == 25);
                    expect_empty_output();
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered") {
                tv.SelectChannel(10);
                tv.SelectChannel(20);
                run_menu_command("SelectPreviousChannel"s);

                THEN("channel is switched to previous") {
                    CHECK(tv.GetChannel() == 10);
                    expect_empty_output();
                }
            }

            AND_WHEN("SelectPreviousChannel command is entered with arguments") {
                run_menu_command("SelectPreviousChannel extra"s);

                THEN("error message is printed") {
                    expect_extra_arguments_error("SelectPreviousChannel"s);
                }
            }

            AND_WHEN("SelectChannel command is entered with extra arguments") {
                run_menu_command("SelectChannel 5 extra"s);

                THEN("error message is printed") {
                    expect_output("Error: too many arguments for SelectChannel command\n"s);
                }
            }
        }
    }
}