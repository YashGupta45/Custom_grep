#include <iostream>
#include <string>
#include <stack>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

// Validate the pattern syntax before matching; throws if invalid.
void validate_pattern(const std::string& pattern) {
    int paren_depth = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        char c = pattern[i];
        if (c == '(') {
            paren_depth++;
            if (i + 1 == pattern.size())
                throw std::runtime_error("Unmatched '(' at end of pattern");
        }
        else if (c == ')') {
            if (paren_depth == 0)
                throw std::runtime_error("Unmatched ')' in pattern");
            paren_depth--;
        }
        else if (c == '[') {
            size_t close_pos = pattern.find(']', i + 1);
            if (close_pos == std::string::npos)
                throw std::runtime_error("Unmatched '[' in pattern");
            i = close_pos; // skip to closing ']'
        }
        else if (c == '\\') {
            if (i + 1 >= pattern.size())
                throw std::runtime_error("Trailing backslash in pattern");
            char next = pattern[i + 1];
            // Allow only these escapes for now
            if (next != 'd' && next != 'w' && !isdigit(next) && next != '[' && next != '\\')
                throw std::runtime_error(std::string("Unsupported escape sequence \\") + next);
            i++; // skip escaped char
        }
        else if (c == '|') {
            // Disallow alternation at start or multiple ||
            if (i == 0 || i + 1 == pattern.size() || pattern[i-1] == '|' || pattern[i+1] == '|')
                throw std::runtime_error("Misplaced alternation operator '|'");
        }
        // You can add more validations as needed
    }
    if (paren_depth != 0)
        throw std::runtime_error("Unmatched '(' in pattern");
}

bool match_digit(const std::string& input_line){
    for(char c : input_line)
        if (isdigit(static_cast<unsigned char>(c))) return true;
    return false;
}

bool match_alphanumeric(const std::string& input_line){
    for(char c : input_line)
        if (isalnum(static_cast<unsigned char>(c))) return true;
    return false;
}

// Compute length of pattern excluding backslashes for quantification purposes
int pattern_Size(const std::string& input){
    int count = 0;
    for(size_t i=0; i < input.size(); ++i)
        if(input[i] != '\\') count++;
    return count;
}

bool positiveMatchGroup(const std::string& input_line, const std::string& pattern, int start, int end){
    std::stack<char> s;
    std::stack<std::pair<char, char>> s_pair;

    int idx = start;
    // Parse character group content (e.g. [a-zA-Z])
    while(idx < end){
        if(idx + 1 < end && pattern[idx + 1] == '-') {
            if (s.empty()) return false;
            char from = s.top(); s.pop();
            char to = pattern[idx + 2];
            s_pair.push({from, to});
            idx += 3;
        } else {
            s.push(pattern[idx]);
            idx++;
        }
    }
    while(!s.empty()){
        char temp = s.top();
        s.pop();
        if(input_line.find(temp) != std::string::npos) return true;
    }
    while(!s_pair.empty()){
        auto temp = s_pair.top();
        s_pair.pop();
        for(char ch = temp.first; ch <= temp.second; ++ch)
            if(input_line.find(ch) != std::string::npos)
                return true;
    }
    return false;
}

bool negativeMatchGroup(const std::string& input_line, const std::string& pattern, int start, int end){
    std::stack<char> s;
    std::stack<std::pair<char,char>> s_pair;

    int idx = start;
    while(idx < end){
        if(idx + 1 < end && pattern[idx + 1] == '-') {
            if (s.empty()) return true;
            char from = s.top(); s.pop();
            char to = pattern[idx + 2];
            s_pair.push({from, to});
            idx += 3;
        } else {
            s.push(pattern[idx]);
            idx++;
        }
    }
    while(!s.empty()){
        char temp = s.top();
        s.pop();
        if(input_line.find(temp) != std::string::npos) return false;
    }
    while(!s_pair.empty()){
        auto temp = s_pair.top();
        s_pair.pop();
        for(char ch = temp.first; ch <= temp.second; ++ch)
            if(input_line.find(ch) != std::string::npos)
                return false;
    }
    return true;
}

bool match_subpattern(const std::string& input_line, const std::string& subpattern) {
    return input_line.find(subpattern) != std::string::npos;
}

bool match_alternation(const std::string& input_line, const std::string& pattern, int start, int end) {
    std::string subpattern = pattern.substr(start, end - start);

    // Support multiple '|' separated alternatives
    size_t pos = 0;
    size_t prev = 0;
    bool matched = false;
    while ((pos = subpattern.find('|', prev)) != std::string::npos) {
        std::string part = subpattern.substr(prev, pos - prev);
        if(match_subpattern(input_line, part))
            matched = true;
        prev = pos + 1;
    }
    // Check last part
    std::string last_part = subpattern.substr(prev);
    if(match_subpattern(input_line, last_part))
        matched = true;

    return matched;
}

// Core pattern matcher that supports groups, quantifiers, escapes, etc.
bool match(const std::string& input_line, const std::string& pattern){
    size_t i = 0;
    int current_group = 1;
    std::unordered_map<int,std::string> group_matches;

    while(i < input_line.size()){
        size_t j = 0;
        size_t temp = i;

        while(j < pattern.size() && temp < input_line.size()){
            if(pattern[j] == '\\'){
                j++;
                if(j >= pattern.size()) break;
                char esc = pattern[j];

                if(esc == 'd'){
                    if(!isdigit(static_cast<unsigned char>(input_line[temp]))) break;
                    temp++;
                } else if(esc == 'w'){
                    if(!isalnum(static_cast<unsigned char>(input_line[temp]))) break;
                    temp++;
                } else if (isdigit(esc)) {
                    int backref_group = esc - '0';
                    if(group_matches.count(backref_group) == 0) return false;
                    std::string expected_match = group_matches[backref_group];
                    if(input_line.substr(temp, expected_match.length()) != expected_match) return false;
                    temp += expected_match.length();
                } else if (esc == '['){
                    // Support escaped character class like \[abc\] (rare)
                    size_t end = pattern.find(']', j);
                    if(end == std::string::npos) return false;
                    if(j + 1 < pattern.size() && pattern[j +1] == '^') {
                        if(!negativeMatchGroup(input_line, pattern, j+2, end)) break;
                    } else {
                        if(!positiveMatchGroup(input_line, pattern, j+1, end)) break;
                    }
                    j = end;
                } else if (esc == '\\') {
                    // To match literal backslash
                    if(input_line[temp] != '\\') break;
                    temp++;
                }
                else{
                    // unsupported escape
                    throw std::runtime_error(std::string("Unsupported escape \\") + esc);
                }
                j++;
                continue;
            }
            else if(pattern[j] == '+'){
                if (j == 0) return false; // no preceding character
                char ch = pattern[j-1];
                if(temp >= input_line.size() || input_line[temp] != ch) break;
                while(temp < input_line.size() && input_line[temp] == ch) temp++;
                j++;
                continue;
            }
            else if(pattern[j] == '?'){
                if (j == 0) return false;
                char ch = pattern[j-1];
                if(temp < input_line.size() && input_line[temp] == ch) temp++;
                j++;
                continue;
            }
            else if(pattern[j] == '.'){
                if(temp >= input_line.size()) break;
                temp++;
                j++;
                continue;
            }
            else if(pattern[j] == '('){
                // find closing )
                int start = j + 1;
                int nesting = 1;
                int k = start;
                for(; k < (int)pattern.size(); ++k) {
                    if(pattern[k] == '(') nesting++;
                    if(pattern[k] == ')') {
                        nesting--;
                        if(nesting == 0) break;
                    }
                }
                if(k == (int)pattern.size()) return false;
                std::string group_pattern = pattern.substr(start, k - start);
                if(temp + pattern_Size(group_pattern) > input_line.size()) return false;
                std::string group_match = input_line.substr(temp, pattern_Size(group_pattern));
                if (!match(group_match, group_pattern)) return false;
                group_matches[current_group] = group_match;
                current_group++;
                temp += pattern_Size(group_pattern);
                j = k + 1;
                continue;
            }
            else if(pattern[j] == '['){
                size_t end = j + 1;
                while(end < pattern.size() && pattern[end] != ']') end++;
                if(end == pattern.size()) return false;

                if(j + 1 < pattern.size() && pattern[j + 1] == '^') {
                    if(!negativeMatchGroup(input_line, pattern, j + 2, end)) break;
                } else{
                    if(!positiveMatchGroup(input_line, pattern, j +1, end)) break;
                }
                j = end + 1;
                continue;
            }
            else { // literal match
                if(temp >= input_line.size()) break;
                if(input_line[temp] != pattern[j]){
                    if(j + 1 < pattern.size() && pattern[j + 1] == '?') {
                        // zero occurrence allowed
                        j += 2;
                        continue;
                    }
                    else break;
                }
                temp++;
                j++;
            }
        }
        if(j == pattern.size()) return true;
        i++;
    }
    return false;
}

bool match_pattern(const std::string& input_line, const std::string& pattern) {
    if (pattern.empty()) return false;

    // Validate pattern syntax
    validate_pattern(pattern);

    if (pattern.length() == 1) {
        return input_line.find(pattern) != std::string::npos;
    }

    if(pattern.find('|') != std::string::npos){
        int pattern_length = pattern.size();
        for (int i = 0; i < pattern_length; ++i) {
            if (pattern[i] == '(') {
                int j = i + 1, paren_count = 1;
                while (j < pattern_length && paren_count > 0) {
                    if (pattern[j] == '(') paren_count++;
                    if (pattern[j] == ')') paren_count--;
                    j++;
                }
                if (paren_count == 0) {
                    return match_alternation(input_line, pattern, i + 1, j - 1);
                }
            } 
        }
    }

    if(pattern[0] == '^'){
        if(pattern.size() - 1 > input_line.size()) return false;
        for(size_t i=1 ; i<pattern.size(); ++i){
            if(i-1 >= input_line.size() || input_line[i-1] != pattern[i]) return false;
        }
        return true;
    }

    if(pattern.back() == '$'){
        if(pattern.size() - 1 > input_line.size()) return false;
        int length = (int)input_line.size() - 1;
        for(int i = (int)pattern.size() - 2; i >= 0 ; --i){
            if(length < 0 || input_line[length] != pattern[i]) return false;
            length--;
        }
        return true;
    }

    if(pattern[0] == '\\' && pattern.length() == 2){
        switch (pattern[1]) {
            case 'd': return match_digit(input_line);
            case 'w': return match_alphanumeric(input_line);
            default:
                throw std::runtime_error("Unhandled pattern " + pattern);
        }
    }

    if(pattern.front() == '[' && pattern.back() == ']'){
        if(pattern[1] == '^'){
            return negativeMatchGroup(input_line,pattern,2,(int)pattern.size()-1);
        }
        else{
            return positiveMatchGroup(input_line,pattern,1,(int)pattern.size()-1);
        }
    }

    if(pattern.size() > 1){
        return match(input_line, pattern);
    }

    throw std::runtime_error("Unhandled pattern " + pattern);
}

int main(int argc, char* argv[]){
    if (argc != 3) {
        std::cerr << "Expected two arguments: -E <pattern>" << std::endl;
        return 1;
    }
    std::string flag = argv[1];
    std::string pattern = argv[2];
    if (flag != "-E") {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    std::string input_line;
    if(!std::getline(std::cin, input_line)) {
        std::cerr << "Failed to read input line" << std::endl;
        return 1;
    }

    try {
        if (match_pattern(input_line, pattern)) {
            return 0; // match success
        } else {
            return 1; // no match
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Pattern error: " << e.what() << std::endl;
        return 1;
    }
}
