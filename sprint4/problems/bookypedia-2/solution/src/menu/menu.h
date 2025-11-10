#pragma once
#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace menu {

    class Menu {
    public:
        using Handler = std::function<std::vector<std::string>(std::istream&)>;

        struct ActionInfo {
            Handler handler;
            std::string args;
            std::string description;
        };

        Menu(std::istream& input, std::ostream& output);

        void AddAction(std::string action_name, std::string args, std::string description,
            Handler handler);

        void Run();

        void ShowInstructions() const;

    private:
        [[nodiscard]] std::vector<std::string> ParseCommand(std::istream& input);

        std::istream& input_;
        std::ostream& output_;
        std::map<std::string, ActionInfo> actions_;
    };

}  // namespace menu