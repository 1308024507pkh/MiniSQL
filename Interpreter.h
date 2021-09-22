/**
 * Interpreter.h
 * The SQL-Language interpreter part of the minisql header file.
 * @Written by YANG Rui
 * @Creation date: Sat June 21 2020
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include "API.h"

using namespace std;

class Interpreter {
public:
    Interpreter() {}

    ~Interpreter() {}

    /*
     * Member variables
    */
    API *ap;//the pointer to api interface
    string fileName;//execfile_name
    int idx = 0;//the index of the sentence

    /*
     * Member Functions
     */

    /**
     * This function classifies and analyzes the sentences entered by the user and
     * call the corresponding member functions.
     * @param s The string entered by the user.
     * @return The status if the sentence done correctly.
     */
    int interpreter(string &s);

private:
    /**
     * This function splits sentences into need words
     * @param s The whole sentence
     * @return the first word we need started from idx in the sentence
     */
    string getWord(string &s);

    /**
     * These functions call the corresponding API model as the function name suggests
     * if the sentence is classified correctly or print ERROR::INFO.
     * @param word The word split from s
     * @param s The whole sentence
     * @return The status if the sentence done correctly
     */
    int CreateTable(string &word, string &s);

    int CreateIndex(string &word, string &s);

    int Select(string &word, string &s);

    int Drop(string &word, string &s);

    int Delete(string &word, string &s);

    int Insert(string &word, string &s);
};

#endif