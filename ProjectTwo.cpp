// ProjectTwo.cpp
// ABCU Advising Assistance Program (single-file solution)
// Reads a CSV of courses, stores them in memory, prints a sorted course list,
// and shows details (title + prerequisites) for a requested course.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// ----------------------------- Utilities -------------------------------------
// Trim leading/trailing spaces and CR/LF
static inline std::string trim(std::string s) {
    auto is_space_or_cntrl = [](unsigned char ch) {
        return std::isspace(ch) || ch == '\r' || ch == '\n';
    };
    while (!s.empty() && is_space_or_cntrl(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && is_space_or_cntrl(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

// Uppercase copy (for normalizing course numbers)
static inline std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// Split a CSV line on commas (no quoted fields in this assignment dataset)
static std::vector<std::string> split_csv_simple(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        parts.push_back(trim(item));
    }
    return parts;
}

// ----------------------------- Data Model ------------------------------------
// Course record
struct Course {
    std::string number;                 // e.g., "CSCI200"
    std::string title;                  // e.g., "Data Structures"
    std::vector<std::string> prereqs;   // e.g., {"CSCI101"}

    Course() = default;
    Course(std::string num, std::string name) : number(std::move(num)), title(std::move(name)) {}
};

// In-memory catalog using an unordered_map for O(1) lookups by course number.
// Sorted output is produced by collecting keys and sorting when needed.
class CourseCatalog {
public:
    void upsert(const Course& c) { data_[upper(c.number)] = c; }
    bool contains(const std::string& number) const { return data_.find(upper(number)) != data_.end(); }
    const Course* get(const std::string& number) const {
        auto it = data_.find(upper(number));
        return (it == data_.end()) ? nullptr : &it->second;
    }
    std::vector<std::string> sorted_numbers() const {
        std::vector<std::string> keys;
        keys.reserve(data_.size());
        for (const auto& kv : data_) keys.push_back(kv.first);
        std::sort(keys.begin(), keys.end());
        return keys;
    }
    void clear() { data_.clear(); }
    bool empty() const { return data_.empty(); }
private:
    std::unordered_map<std::string, Course> data_;
};

// ------------------------------ Loading --------------------------------------
// Reads the CSV file into the provided catalog.
// Returns true on success; false with error message on failure.
// Uses a temporary catalog to avoid partially mutating on errors.
bool load_catalog_from_csv(const std::string& file_path, CourseCatalog& out_catalog, std::string& err) {
    std::ifstream fin(file_path);
    if (!fin) {
        err = "Could not open file: " + file_path;
        return false;
    }

    CourseCatalog temp;
    std::string line;
    size_t line_no = 0;

    while (std::getline(fin, line)) {
        ++line_no;
        line = trim(line);
        if (line.empty()) continue;  // skip blank lines

        auto fields = split_csv_simple(line);
        if (fields.size() < 2) {
            std::ostringstream oss;
            oss << "Parse error on line " << line_no << ": need at least courseNumber and courseTitle.";
            err = oss.str();
            return false;
        }

        std::string number = upper(fields[0]);
        std::string title  = fields[1];

        if (number.empty() || title.empty()) {
            std::ostringstream oss;
            oss << "Invalid data on line " << line_no << ": empty course number or title.";
            err = oss.str();
            return false;
        }

        Course c(number, title);

        // Any remaining fields are prerequisites (normalize to uppercase)
        for (size_t i = 2; i < fields.size(); ++i) {
            std::string prereq = upper(fields[i]);
            if (!prereq.empty()) c.prereqs.push_back(prereq);
        }

        temp.upsert(c);
    }

    // Success: commit populated temp catalog
    out_catalog = std::move(temp);
    return true;
}

// ---------------------------- Presentation -----------------------------------
void print_welcome() {
    std::cout << "Welcome to the course planner.\n\n";
}

void print_menu() {
    std::cout << "  1. Load Data Structure.\n"
              << "  2. Print Course List.\n"
              << "  3. Print Course.\n"
              << "  9. Exit\n\n"
              << "What would you like to do? ";
}

// Prints the full, alphanumeric course list.
void print_course_list(const CourseCatalog& catalog) {
    auto numbers = catalog.sorted_numbers();
    std::cout << "Here is a sample schedule:\n";
    for (const auto& num : numbers) {
        const Course* c = catalog.get(num);
        if (c) std::cout << c->number << ", " << c->title << "\n";
    }
    std::cout << "\n";
}

// Prints a single course's title + prerequisites.
void print_course_details(const CourseCatalog& catalog, const std::string& user_input_number) {
    std::string number = upper(user_input_number);
    const Course* c = catalog.get(number);
    if (!c) {
        std::cout << number << " was not found.\n\n";
        return;
    }

    std::cout << c->number << ", " << c->title << "\n";

    if (c->prereqs.empty()) {
        std::cout << "Prerequisites: None\n\n";
        return;
    }

    std::cout << "Prerequisites: ";
    for (size_t i = 0; i < c->prereqs.size(); ++i) {
        const std::string& pnum = c->prereqs[i];
        const Course* pc = catalog.get(pnum);
        if (pc) std::cout << pc->number;
        else    std::cout << pnum;
        if (i + 1 < c->prereqs.size()) std::cout << ", ";
    }
    std::cout << "\n\n";
}

// ------------------------------- Main ----------------------------------------
int main() {
    CourseCatalog catalog;
    bool running = true;

    print_welcome();

    while (running) {
        print_menu();

        std::string option_raw;
        if (!std::getline(std::cin, option_raw)) break;
        option_raw = trim(option_raw);

        if (option_raw.empty()) {
            std::cout << "Please enter a menu option.\n\n";
            continue;
        }

        // Accept single-digit numeric input; treat others as invalid.
        if (option_raw.size() > 2) {
            std::cout << option_raw << " is not a valid option.\n\n";
            continue;
        }

        int option = -1;
        try {
            option = std::stoi(option_raw);
        } catch (...) {
            std::cout << option_raw << " is not a valid option.\n\n";
            continue;
        }

        switch (option) {
            case 1: { // Load
                std::cout << "Enter the file name to load (e.g., CS 300 ABCU_Advising_Program_Input.csv): ";
                std::string filename;
                if (!std::getline(std::cin, filename)) {
                    std::cout << "Input cancelled.\n\n";
                    break;
                }
                filename = trim(filename);

                if (filename.empty()) {
                    std::cout << "File name cannot be empty.\n\n";
                    break;
                }

                std::string err;
                CourseCatalog newCatalog;
                if (load_catalog_from_csv(filename, newCatalog, err)) {
                    catalog = std::move(newCatalog);
                    std::cout << "Data loaded successfully (" 
                              << catalog.sorted_numbers().size() << " courses).\n\n";
                } else {
                    std::cout << "Error: " << err << "\n\n";
                }
                break;
            }

            case 2: { // Print Course List
                if (catalog.empty()) {
                    std::cout << "Please load the data structure first (option 1).\n\n";
                } else {
                    print_course_list(catalog);
                }
                break;
            }

            case 3: { // Print Course Details
                if (catalog.empty()) {
                    std::cout << "Please load the data structure first (option 1).\n\n";
                } else {
                    std::cout << "What course do you want to know about? ";
                    std::string number;
                    if (!std::getline(std::cin, number)) {
                        std::cout << "Input cancelled.\n\n";
                        break;
                    }
                    number = trim(number);
                    if (number.empty()) {
                        std::cout << "Course number cannot be empty.\n\n";
                        break;
                    }
                    print_course_details(catalog, number);
                }
                break;
            }

            case 9: { // Exit
                std::cout << "Thank you for using the course planner!\n";
                running = false;
                break;
            }

            default:
                std::cout << option << " is not a valid option.\n\n";
        }
    }

    return 0;
}
