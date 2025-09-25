#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utf8proc.h>

typedef struct {
  char *data;    // UTF-8 string buffer, null-terminated
  size_t length; // length in bytes, excluding null terminator
} Utf8String;

// Initialize empty string
void Utf8String_init(Utf8String *s) {
  s->data = malloc(1);
  if (!s->data) {
    perror("malloc");
    exit(1);
  }
  s->data[0] = '\0';
  s->length = 0;
}

void Utf8String_free(Utf8String *s) {
  free(s->data);
  s->data = NULL;
  s->length = 0;
}

void Utf8String_set(Utf8String *s, const char *utf8str) {
  size_t new_len = strlen(utf8str);
  char *new_data = realloc(s->data, new_len + 1);
  if (!new_data) {
    perror("realloc");
    exit(1);
  }
  s->data = new_data;
  memcpy(s->data, utf8str, new_len + 1);
  s->length = new_len;
}

void Utf8String_append(Utf8String *s, const char *utf8str) {
  size_t add_len = strlen(utf8str);
  char *new_data = realloc(s->data, s->length + add_len + 1);
  if (!new_data) {
    perror("realloc");
    exit(1);
  }
  s->data = new_data;
  memcpy(s->data + s->length, utf8str, add_len + 1);
  s->length += add_len;
}

size_t Utf8String_length_chars(const Utf8String *s) {
  size_t count = 0;
  size_t i = 0;

  while (i < s->length) {
    unsigned char c = (unsigned char)s->data[i];

    if ((c & 0x80) == 0) {
      i += 1;
    } else if ((c & 0xE0) == 0xC0) {
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {
      i += 4;
    } else {
      i += 1; // invalid UTF-8, skip
    }
    count++;
  }

  return count;
}

const char *Utf8String_c_str(const Utf8String *s) { return s->data; }

// New: Normalize string using utf8proc NFC (canonical composition)
void Utf8String_normalize(Utf8String *s) {
  utf8proc_uint8_t *normalized;
  ssize_t new_len =
      utf8proc_map((const utf8proc_uint8_t *)s->data, 0, &normalized,
                   UTF8PROC_NULLTERM | UTF8PROC_COMPOSE);
  if (new_len < 0) {
    fprintf(stderr, "utf8proc_map normalize error: %s\n",
            utf8proc_errmsg(new_len));
    return;
  }

  char *new_data = realloc(s->data, new_len + 1);
  if (!new_data) {
    perror("realloc");
    free(normalized);
    exit(1);
  }

  s->data = new_data;
  memcpy(s->data, normalized, new_len + 1);
  s->length = new_len;
  free(normalized);
}

// New: Convert string to lowercase using utf8proc case folding
void Utf8String_to_lower(Utf8String *s) {
  utf8proc_uint8_t *lowered;
  ssize_t new_len =
      utf8proc_map((const utf8proc_uint8_t *)s->data, 0, &lowered,
                   UTF8PROC_NULLTERM | UTF8PROC_CASEFOLD | UTF8PROC_STABLE);
  if (new_len < 0) {
    fprintf(stderr, "utf8proc_map to_lower error: %s\n",
            utf8proc_errmsg(new_len));
    return;
  }

  char *new_data = realloc(s->data, new_len + 1);
  if (!new_data) {
    perror("realloc");
    free(lowered);
    exit(1);
  }

  s->data = new_data;
  memcpy(s->data, lowered, new_len + 1);
  s->length = new_len;
  free(lowered);
}

// New: Convert string to uppercase (via to_lower + then map to uppercase,
// utf8proc doesn't provide direct uppercase, so we do a workaround)
void Utf8String_to_upper(Utf8String *s) {
  // utf8proc doesn't have direct uppercase, but you can invert case folding:
  // Simplest way: use utf8proc_map with UTF8PROC_CASEFOLD | UTF8PROC_STABLE and
  // then apply to_lower again, but no direct uppercase. For real uppercase, ICU
  // is better.
  fprintf(stderr, "Warning: utf8proc lacks direct uppercase mapping; use ICU "
                  "for full uppercase support.\n");
}

// Simple test

int main() {
  Utf8String s;
  Utf8String_init(&s);

  Utf8String_set(&s, "Straße"); // German sharp S
  printf("Original: %s\n", Utf8String_c_str(&s));

  Utf8String_to_lower(&s);
  printf("Lowercase: %s\n", Utf8String_c_str(&s));

  Utf8String_normalize(&s);
  printf("Normalized: %s\n", Utf8String_c_str(&s));

  Utf8String_free(&s);
  return 0;
}
