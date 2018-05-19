// 1. to hound the app into source code files ✓
// 2. buld an array of tokens
// 3. collapse {tokens} → {tokens, frequency}, where each token appears once.
// ... profit! Actually, let's look at the results of 3-rd step before proceeding.

use std::env;
use std::fs::File;
use std::io::prelude::*;

enum TokenTypes {
    // todo: do I really need to take into account newlines in tokens?
    Uninitialized,
    AlphaNumeric, // alphanumeric, _
    Specials // non-alphanumeric, but specials, _, spaces, newlines.
}

fn tokenize(ch: char, mut state: Vec<String>,
            type_processed: TokenTypes) -> Vec<String> {
    match type_processed {
        TokenTypes::AlphaNumeric => {
            if ch.is_alphanumeric() {
                state.last_mut().unwrap().push(ch);
                return state;
            } else {
                state.push(String::new()); // start a new token
                return tokenize(ch, state, TokenTypes::Specials);
            }
        }
        TokenTypes::Specials => {
            if !ch.is_alphanumeric() {
                state.last_mut().unwrap().push(ch);
                return state;
            } else {
                state.push(String::new()); // start a new token
                return tokenize(ch, state, TokenTypes::AlphaNumeric);
            }
        }
        TokenTypes::Uninitialized => {
            state.push(String::new()); // start a new token
            if ch.is_alphanumeric() {
                return tokenize(ch, state, TokenTypes::AlphaNumeric);
            } else {
                return tokenize(ch, state, TokenTypes::Specials);
            }
        }
    }
}

fn main() {
    println!("Please, enter filenames to read form");
    let mut input_data = String::new();
    for filename in env::args().skip(1) {
        println!("Opening a file {:?}", filename);
        let mut file = File::open(filename).expect("file error");
        file.read_to_string(&mut input_data).expect("Error reading a file");
        print!("{}", input_data);
    }
}
