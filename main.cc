#include <algorithm>
#include <conio.h>
#include <functional>
#include <stdio.h>
#include <string.h>

#include "SDL.h"
#include "SDL_ttf.h" 

#define SIZE 256
#define MAX_LINES 65536

// Internal type defining how we store the buffer's text data.
struct Storage {
  char* data;
};

// Internal type defining a mark's position in a buffer.
struct Location {
  int line;
  int column;
};

// A mark is like a cursor in the file.
struct Mark {
  // TODO: Maybe move this linked list stuff into a MarkNode struct instead.
  Mark* next_mark;
 
  int id;
  Location position;
  bool fixed;
};

struct Mode {
  // TODO: Maybe move this linked list stuff into a ModeNode struct instead.
  Mode* next_mode;

  // Unique identifier for this mode.
  char* mode_name;

  // A function to execute whenever the command set for a buffer needs to be
  // created (or recreated).
  std::function<bool()> on_add;
};

// A buffer contains text, marks to edit that text, and info about the file
// that text was loaded from / should be saved to.
struct Buffer {
  // The next buffer in the world.
  // TODO: Maybe move this linked list stuff into a BufferNode struct instead.
  Buffer* next_buffer;
  std::string buffer_name;

  // List of marks that are editing the buffer.
  Mark* marks;

  // Contents of the file.
  Storage* contents;

  // File metadata.
  std::string file_name;
  long file_time;
  bool modified;

  // Editing modes of the file.
  Mode* modes;

  // TODO: Maybe add these fields too if we need them.
  // int current_line;
  // int num_chars;
  // int num_lines;
};

// The world is a linked list of all buffers.
struct World {
  Buffer* buffers;
  Buffer* current_buffer;
};

bool IsPhraseBreak(char c) {
  return c == ' ' || c == ';' || c == '-' || c == '{' || c == '}';
}

int main(int argc, char* argv[]) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Could not init SDL: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
      "Pad",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      0, 0,
      SDL_WINDOW_FULLSCREEN);
  if (window == nullptr) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "error", SDL_GetError(),
                             nullptr);
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, 0);
  if (renderer == nullptr) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "error", SDL_GetError(), window);
    return 1;
  }

  if (TTF_Init() < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "text init", TTF_GetError(), window);
    return 1;
  }

  TTF_Font* sans = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", 24);
  if (sans == nullptr) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "text error", TTF_GetError(), window);
    return 1;
  }

  // Setup text input state.
  char buffer[SIZE];
  char* current = buffer;
  char* composition;
  size_t cursor = 0;
  size_t line = 0;
  size_t selection_len = 0;

  buffer[0] = '\0';

  SDL_StartTextInput();

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  // Game loop.
  bool running = true;
  bool minimized = false;
  while (running) {
    // Event Handling
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      // Quit
      if (event.type == SDL_QUIT) {
        running = false;
      }
      if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_SHOWN) {
          minimized = false;
        } else if (event.window.event == SDL_WINDOWEVENT_HIDDEN) {
          minimized = true;
        }
      }
      // Special KeyDown
      if (event.type == SDL_KEYDOWN) {
        // Escape
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          running = false;
        }
        // Left
        if (event.key.keysym.sym == SDLK_LEFT) {
          cursor = std::max(static_cast<size_t>(0), cursor - 1);
        }
        // Right
        if (event.key.keysym.sym == SDLK_RIGHT) {
          cursor = std::min(strlen(buffer), cursor + 1);
        }
        // Backspace
        if (event.key.keysym.sym == SDLK_BACKSPACE && cursor > 0) {
          // Move back to the part we want to delete.
          int stop = cursor - 1;
          if (SDL_GetModState() & KMOD_CTRL) {
            while (stop > 0 && !IsPhraseBreak(buffer[stop])) {
              stop -= 1;
            }
          }
          // Copy characters from cursor up until end of buffer.
          int distance = strlen(buffer) - cursor;

          strncpy(&buffer[stop], &buffer[cursor], distance);
          buffer[stop + distance] = '\0';
          cursor = stop;
        }
      }
      // TextInput
      if (event.type == SDL_TEXTINPUT) {
        if (cursor >= strlen(buffer)) {
          // Append
          strncat(buffer, event.text.text, SIZE - strlen(buffer) - 1);
          cursor = strlen(buffer);
        } else {
          // Insert
          int temp_size = strlen(buffer) - cursor;
          char* temp = new char[temp_size + 1];

          strncpy(temp, &buffer[cursor], temp_size);
          temp[temp_size] = '\0';

          buffer[cursor] = '\0';
          strncat(buffer, event.text.text, SIZE - strlen(buffer) - 1);
          cursor = strlen(buffer);
          strncat(buffer, temp, SIZE - strlen(buffer) - 1);

          delete temp;
        }
      }
      // TextEditing
      if (event.type == SDL_TEXTEDITING) {
        composition = event.edit.text;
        cursor = event.edit.start;
        selection_len = event.edit.length;
      }
    }

    if (minimized) {
      continue;
    }

    // BEGIN RENDERING
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Rect debug_cursor = {200 + cursor * 13, 12, 1, 24};
    SDL_Rect debug_buffer_size = {200, 12, strlen(buffer) * 13, 24};
    //SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
    //SDL_RenderFillRect(renderer, &debug_buffer_size);
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderFillRect(renderer, &debug_cursor);

    if (strlen(buffer) > 0) {
      SDL_Surface* text_surface = TTF_RenderText_Solid(
          sans, buffer, /* fg= */ {255, 255, 255});
      if (text_surface == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "text surface error",
                                 SDL_GetError(), window);
        return 1;
      }

      SDL_Texture* text_texture = SDL_CreateTextureFromSurface(
          renderer, text_surface);
      if (text_texture == nullptr) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "text texture error",
                                 SDL_GetError(), window);
        return 1;
      }

      SDL_Rect text_rect = {200, 12, text_surface->w, text_surface->h};
      SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);

      // TODO: why does this prevent the reticle from rendering?
      // SDL_DestroyTexture(text_texture);
      // SDL_FreeSurface(text_surface);
    }

    SDL_RenderPresent(renderer);
    // END RENDERING
  }

  TTF_Quit();

  SDL_StopTextInput();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
