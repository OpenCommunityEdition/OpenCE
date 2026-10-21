//! A command line's words, as a shell splits them: by spaces, with
//! double or single quotes around a word that has spaces, and a backslash
//! before a character taken as it is.

use crate::CommandError;

/// A line's words.
pub fn parse_words(line: &str) -> Result<Vec<String>, CommandError> {
    let mut words = Vec::new();
    let mut word = String::new();
    let mut in_word = false;
    let mut quote = None;
    let mut chars = line.chars();
    while let Some(c) = chars.next() {
        match (quote, c) {
            (None, c) if c.is_whitespace() => {
                if in_word {
                    words.push(std::mem::take(&mut word));
                    in_word = false;
                }
            }
            (None, '"' | '\'') => {
                quote = Some(c);
                in_word = true;
            }
            (Some(q), c) if c == q => quote = None,
            (Some('\''), c) => word.push(c),
            (_, '\\') => {
                let Some(next) = chars.next() else {
                    return Err(CommandError::Syntax("a backslash ends the line".into()));
                };
                word.push(next);
                in_word = true;
            }
            (_, c) => {
                word.push(c);
                in_word = true;
            }
        }
    }
    if quote.is_some() {
        return Err(CommandError::Syntax("a quote is not closed".into()));
    }
    if in_word {
        words.push(word);
    }
    Ok(words)
}

/// A line's commands: split at each `;` outside quotes and parentheses,
/// each trimmed, the empty ones left out.
pub fn split_commands(line: &str) -> Vec<&str> {
    let mut commands = Vec::new();
    let mut start = 0;
    let mut depth = 0u32;
    let mut quote = None;
    let mut escaped = false;
    for (at, c) in line.char_indices() {
        if escaped {
            escaped = false;
            continue;
        }
        match (quote, c) {
            (_, '\\') if quote != Some('\'') => escaped = true,
            (None, '"' | '\'') => quote = Some(c),
            (Some(q), c) if c == q => quote = None,
            (None, '(') => depth += 1,
            (None, ')') => depth = depth.saturating_sub(1),
            (None, ';') if depth == 0 => {
                commands.push(line[start..at].trim());
                start = at + 1;
            }
            _ => {}
        }
    }
    commands.push(line[start..].trim());
    commands.retain(|c| !c.is_empty());
    commands
}
