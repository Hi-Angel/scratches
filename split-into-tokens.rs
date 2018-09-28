use std::env;
use std::fs::File;
use std::io::prelude::*;
use std::collections::HashMap;

// arbitrary choosen lowest percentage for a word that we'd consider a keyword
static KEYW_FREQ: f32 = 0.1;
static WIN_SZ: i16 = 6;

enum TokenTypes {
    // todo: do I really need to take into account newlines in tokens?
    SkipWhitespace, // a whitespace that does not belong to Specials. Can be used as initial state
    AlphaNumeric, // alphanumeric, _
    Specials // non-alphanumeric, but specials, _, spaces, newlines.
        //todo: add "Finished" to move last processee from processee
}

type Freq       = u32;
type Token      = String;
type TokensInfo = HashMap<Token, Freq>;

struct TokenizeState {
    tokens: TokensInfo,
    text: Vec<Token>,
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
                return tokenize(TokenizeState{processee: String::new(),
                                              type_processed: if ch.is_whitespace() {TokenTypes::SkipWhitespace} else {TokenTypes::Specials},
                                              ..state}, ch);
            }
        }
        TokenTypes::Specials => {
            if !ch.is_alphanumeric() { // whitespace included
                state.processee.push(ch);
                return state;
            } else { // finish a token, start a new one
                let solid_token = state.processee.trim().to_string();
                if solid_token.len() != 0 { // ignore whitespace-only tokens
                    *state.tokens.entry(solid_token).or_insert(0) += 1;
                }
                return tokenize(TokenizeState{processee: String::new(),
                                              type_processed: TokenTypes::AlphaNumeric, ..state}, ch);
            }
        }
        TokenTypes::SkipWhitespace => {
            if ch.is_whitespace() {
                return state;
            } if ch.is_alphanumeric() {
                return tokenize(TokenizeState{type_processed: TokenTypes::AlphaNumeric, ..state}, ch);
            } else {
                return tokenize(TokenizeState{type_processed: TokenTypes::Specials, ..state}, ch);
            }
        }
    }
}

// fn collect_windows(text: &str, tokens: TokensInfo) -> Vec<Box<(&String, &Freq)>>{
//     // text is measured in tokens, so we gotta break text to array of tokens. Then:
//     // for window in text:
//     //     for token in window
//     //         let refToken = getRef freqTable token
// }

// todo: make type Window to refer a single pair of the map TokensInfo.

fn main() {
    if env::args().len() <= 1 {
        println!("Please, enter filenames to read form");
    }
    let mut state = TokenizeState{ tokens: HashMap::new(),
                                   text: Vec::new(),
                                   processee: String::new(),
                                   type_processed: TokenTypes::SkipWhitespace };
    for filename in env::args().skip(1) {
        println!("Opening a file {:?}", filename);
        let mut file = File::open(filename).expect("file error");
        let mut input_data = String::new();
        file.read_to_string(&mut input_data).expect("Error reading a file");
        state = input_data.chars().fold(state, tokenize);
    }
    let mut sorted: Vec<(&String, &Freq)> = state.tokens.iter().collect();
    sorted.sort_by(|a, b| b.1.cmp(a.1));
    for &(key, freq) in sorted.iter() {
        println!("{:04}: {}", (*freq) as f32 / (sorted.len()) as f32, key);
    }
    // todo: rust API is not yet ready for that kind of task https://stackoverflow.com/questions/51459146/how-do-i-create-a-vec-of-entries-from-a-hashmap
}

// getWindowNonKeywords :: Int -> Slice -> KeywordsPattern
// getWindowNonKeywords start = toIndices $ getNonKeyTokensAt WIN_SZ start
//
// getAllWindows :: Int -> Slice -> [KeywordsPattern]
// getAllWindows start = map (\start1 -> getWindowNonKeywords $ start + start1) [0-WIN_SZ..WIN_SZ]
//
//
// completeAt at = do
//   let keywordsPatterns = getAllWindows 8 at
//       foo = getCompletionsProbabilitySortedByPattern keywordsPatterns
//
// -- let's make all possible windows (where distinct window determined by "keywords"
// -- and repeating non-keywords). Then, to get completion we gonna check probable
// -- windows. Typing a letter would sort probablities out.

// -- 1. collect all tokens, and determine which ones "keywords"
// -- 2. collect all possible windows, and enumerate non-keywords by index as per paper
// -- 3. create a separate list of pattern-squashed windows through blanking non-keywords
// -- 4. there I probably have to assign into every pattern-squashed window its
// -- probability relative to patterns around, by I dunno yet what to do next. Let's see
// -- what we get there
// todo: fuzzy window len. It could be worthwhile to store variants of windows of different len, and take the one with most freq.
