FPS = 40;
[R, G, B] = acquireVideo('mp4/vid.mp4');
process(R, FPS);
process(G, FPS);
process(B, FPS);