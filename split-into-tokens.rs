// 1. to hound the app into source code files ✓
// 2. buld an array of tokens
// 3. collapse {tokens} → {tokens, frequency}, where each token appears once.
// ... profit! Actually, let's look at the results of 3-rd step before proceeding.

use std::env;
use std::fs::File;
use std::io::prelude::*;

#[derive(Debug)]
enum TokenTypes {
    // todo: do I really need to take into account newlines in tokens?
    Uninitialized,
    AlphaNumeric, // alphanumeric, _
    Specials // non-alphanumeric, but specials, _, spaces, newlines.
}

#[derive(Debug)]
struct TokenizeState {
    vec: Vec<String>,
    type_processed: TokenTypes
}

fn tokenize(mut state: TokenizeState, ch: char) -> TokenizeState {
    match state.type_processed {
        TokenTypes::AlphaNumeric => {
            if ch.is_alphanumeric() || ch == '_' {
                state.vec.last_mut().unwrap().push(ch);
                return state;
            } else {
                state.vec.push(String::new()); // start a new token
                return tokenize(TokenizeState{type_processed: TokenTypes::Specials, ..state}, ch);
            }
        }
        TokenTypes::Specials => {
            if !ch.is_alphanumeric() {
                state.vec.last_mut().unwrap().push(ch);
                return state;
            } else {
                state.vec.push(String::new()); // start a new token
                return tokenize(TokenizeState{type_processed: TokenTypes::AlphaNumeric, ..state}, ch);
            }
        }
        TokenTypes::Uninitialized => {
            state.vec.push(String::new()); // start a new token
            if ch.is_alphanumeric() {
                return tokenize(TokenizeState{type_processed: TokenTypes::AlphaNumeric, ..state}, ch);
            } else {
                return tokenize(TokenizeState{type_processed: TokenTypes::Specials, ..state}, ch);
            }
        }
    }
}

fn main() {
    println!("Please, enter filenames to read form");
    let mut input_data = String::new();
    let mut state = TokenizeState{vec: Vec::new(), type_processed: TokenTypes::Uninitialized};
    for filename in env::args().skip(1) {
        println!("Opening a file {:?}", filename);
        let mut file = File::open(filename).expect("file error");
        file.read_to_string(&mut input_data).expect("Error reading a file");
        state = input_data.chars().fold(state, tokenize);
    }
    for token in state.vec {
        println!("{}", token);
    }
}
