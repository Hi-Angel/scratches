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

struct TokenizeState {
    vec: Vec<String>,
    type_processed: TokenTypes
}

fn tokenize(text: String) -> Vec<String> {
    let mut state = TokenizeState{ vec: Vec::new(), type_processed: TokenTypes::Uninitialized };
    for char in text.chars() {
        match state.type_processed {
            TokenTypes::AlphaNumeric => {
                if is_alphanumeric(char) {
                    state.vec.last().push_str(&char);
                } else // recursion?
            }
        }
    }
    return state.vec;
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
