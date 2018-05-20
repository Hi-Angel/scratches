// 1. to hound the app into source code files ✓
// 2. buld an array of tokens ✓
// 3. collapse {tokens} → {tokens, frequency}, where each token appears once. ✓
// ... profit! Actually, let's look at the results of 3-rd step before proceeding.

use std::env;
use std::fs::File;
use std::io::prelude::*;
use std::collections::BTreeMap;

#[derive(Debug)]
enum TokenTypes {
    // todo: do I really need to take into account newlines in tokens?
    Uninitialized,
    AlphaNumeric, // alphanumeric, _
    Specials // non-alphanumeric, but specials, _, spaces, newlines.
        //todo: add "Finished" to move last processee from processee
}

type Freq = u32;

#[derive(Debug)]
struct TokenizeState {
    tokens: BTreeMap<String, Freq>,
    processee: String,
    type_processed: TokenTypes
}

fn tokenize(mut state: TokenizeState, ch: char) -> TokenizeState {
    match state.type_processed {
        TokenTypes::AlphaNumeric => {
            if ch.is_alphanumeric() || ch == '_' {
                state.processee.push(ch);
                return state;
            } else { // finish a token, start a new one
                *state.tokens.entry(state.processee.clone()).or_insert(0) += 1;
                state.processee = String::new();
                return tokenize(TokenizeState{type_processed: TokenTypes::Specials, ..state}, ch);
            }
        }
        TokenTypes::Specials => {
            if !ch.is_alphanumeric() {
                state.processee.push(ch);
                return state;
            } else { // finish a token, start a new one
                *state.tokens.entry(state.processee.clone()).or_insert(0) += 1;
                state.processee = String::new();
                return tokenize(TokenizeState{type_processed: TokenTypes::AlphaNumeric, ..state}, ch);
            }
        }
        TokenTypes::Uninitialized => {
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
    let mut state = TokenizeState{tokens: BTreeMap::new(), processee: String::new(), type_processed: TokenTypes::Uninitialized};
    for filename in env::args().skip(1) {
        println!("Opening a file {:?}", filename);
        let mut file = File::open(filename).expect("file error");
        file.read_to_string(&mut input_data).expect("Error reading a file");
        state = input_data.chars().fold(state, tokenize);
    }
    let mut sorted: Vec<(&String, &Freq)> = state.tokens.iter().collect();
    sorted.sort_by(|a, b| b.1.cmp(a.1));
    for &(key, value) in sorted.iter() {
        println!("{:04}: {}", value, key);
    }
}
