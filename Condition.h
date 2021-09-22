//Condition.h

#ifndef __Condition__
#define __Condition__

#include <string>
#include <sstream>

using namespace std;

class Condition {
public:
    const static int OPERATOR_EQUAL = 0;      // "="
    const static int OPERATOR_NOT_EQUAL = 1;  // "<>"
    const static int OPERATOR_LESS = 2;       // "<"
    const static int OPERATOR_MORE = 3;       // ">"
    const static int OPERATOR_LESS_EQUAL = 4; // "<="
    const static int OPERATOR_MORE_EQUAL = 5; // ">="
    Condition(string a, string v, int o) {
        attributeName = a;
        value = v;
        operate = o;
    }

    string attributeName;
    string value;
    int operate;

    bool ifRight(int content) {
        stringstream ss;
        ss << value;
        int myContent;
        ss >> myContent;
        switch (operate) {
            case Condition::OPERATOR_EQUAL:
                return content == myContent;
                break;
            case Condition::OPERATOR_NOT_EQUAL:
                return content != myContent;
                break;
            case Condition::OPERATOR_LESS:
                return content < myContent;
                break;
            case Condition::OPERATOR_MORE:
                return content > myContent;
                break;
            case Condition::OPERATOR_LESS_EQUAL:
                return content <= myContent;
                break;
            case Condition::OPERATOR_MORE_EQUAL:
                return content >= myContent;
                break;
            default:
                return true;
                break;
        }
    }

    bool ifRight(float content) {
        stringstream ss;
        ss << value;
        float myContent;
        ss >> myContent;
        switch (operate) {
            case Condition::OPERATOR_EQUAL:
                return content == myContent;
                break;
            case Condition::OPERATOR_NOT_EQUAL:
                return content != myContent;
                break;
            case Condition::OPERATOR_LESS:
                return content < myContent;
                break;
            case Condition::OPERATOR_MORE:
                return content > myContent;
                break;
            case Condition::OPERATOR_LESS_EQUAL:
                return content <= myContent;
                break;
            case Condition::OPERATOR_MORE_EQUAL:
                return content >= myContent;
                break;
            default:
                return true;
                break;
        }
    }

    bool ifRight(string content) {
        string myContent = value;
        switch (operate) {
            case Condition::OPERATOR_EQUAL:
                return content == myContent;
                break;
            case Condition::OPERATOR_NOT_EQUAL:
                return content != myContent;
                break;
            case Condition::OPERATOR_LESS:
                return content < myContent;
                break;
            case Condition::OPERATOR_MORE:
                return content > myContent;
                break;
            case Condition::OPERATOR_LESS_EQUAL:
                return content <= myContent;
                break;
            case Condition::OPERATOR_MORE_EQUAL:
                return content >= myContent;
                break;
            default:
                return true;
                break;
        }
    }
};

#endif
