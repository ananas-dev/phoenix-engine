# Clippy NNUE

Strong engine for the fenix board game

```
 __                 
/  \        _____________
|  |       /             \
@  @       | Good luck   |
|| ||   <--| have fun!   |
|| ||      \_____________/
|\_/|     
\___/
```

## How to use

Clippy uses the UGI protocol: https://github.com/kz04px/cutegames/blob/master/ugi.md.

A web ui is under construction :)

## Network

The evaluation function is a `(336 -> 128)x2 -> 2` NNUE, the first output is for the setup phase and the second one is for the rest of game.
The network has been trained using the amazing bullet library.


## Special thanks

- The bitboard chess engine YouTube series: https://www.youtube.com/watch?v=QUNP-UjujBM&list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs
- The chess programming wiki: https://www.chessprogramming.org
- The bullet ML library: https://github.com/jw1912/bullet
