#include <SDL/SDL.h>

#define LARGEUR_PX 600
#define HAUTEUR_PX 400

#define MAX_ITER 1000
#define CHANGEMENT_COULEUR .01

#define MASQUE .3

typedef struct Repere {
        SDL_Surface* s;
        double x1, x2, y1, y2;
} Repere;

void ecranVersRepere(const Repere* const r, const int x, const int y, double* const rx, double* const ry) {

        *rx = (x * r->x2 + (r->s->w - x) * r->x1) / r->s->w;
        *ry = (y * r->y1 + (r->s->h - y) * r->y2) / r->s->h;

}

Uint32* pixel(SDL_Surface* const s, const int x, const int y) {

        return &((Uint32*)s->pixels)[y * s->w + x];

}

void couleur(double teinte, Uint8* const rouge, Uint8* const vert, Uint8* const bleu) {

        while (teinte > 1)
                teinte -= 1;

        teinte *= 6;

        if (teinte < 2) {
                *bleu = 0;
                if (teinte < 1) {
                        *rouge = 255;
                        *vert = teinte * 255;
                } else {
                        *vert = 255;
                        *rouge = (2 - teinte) * 255;
                }
        } else if (teinte < 4) {
                *rouge = 0;
                if (teinte < 3) {
                        *vert = 255;
                        *bleu = (teinte - 2) * 255;
                } else {
                        *bleu = 255;
                        *vert = (4 - teinte) * 255;
                }
        } else {
                *vert = 0;
                if (teinte < 5) {
                        *bleu = 255;
                        *rouge = (teinte - 4) * 255;
                } else {
                        *rouge = 255;
                        *bleu = (6 - teinte) * 255;
                }
        }

}

int divergence(const double x, const double y, const int maxIter) {

        double zx = 0, zy = 0;

        int i;
        for (i = 0; i < maxIter; ++i) {

                if (zx * zx + zy * zy > 4)
                        return i + 1;

                double re = zx * zx - zy * zy;
                double im = 2 * zx * zy;

                zx = re + x;
                zy = im + y;

        }

        return 0;

}

void mandelbrot(Repere* const r) {

        int x, y;
        for (y = 0; y < r->s->h; ++y) {
                for (x = 0; x < r->s->w; ++x) {

                        double rx, ry;
                        ecranVersRepere(r, x, y, &rx, &ry);

                        const int iter = divergence(rx, ry, MAX_ITER);

                        if (iter) {

                                Uint8 rouge, vert, bleu;
                                couleur(iter * CHANGEMENT_COULEUR, &rouge, &vert, &bleu);
                                *pixel(r->s, x, y) = SDL_MapRGB(r->s->format, rouge, vert, bleu);

                        } else *pixel(r->s, x, y) = SDL_MapRGB(r->s->format, 0, 0, 0);

                }
        }

}

Uint32 surbrillance(const Repere* const r, const int x, const int y, SDL_PixelFormat* const formatDst) {

        Uint8 rouge, vert, bleu;
        SDL_GetRGB(*pixel(r->s, x, y), r->s->format, &rouge, &vert, &bleu);

        rouge = 255 - rouge;
        vert = 255 - vert;
        bleu = 255 - bleu;

        return SDL_MapRGB(formatDst, rouge, vert, bleu);

}

Uint32 assombrir(const Repere* const r, const int x, const int y, SDL_PixelFormat* formatDest) {

        Uint8 rouge, vert, bleu;
        SDL_GetRGB(*pixel(r->s, x, y), r->s->format, &rouge, &vert, &bleu);

        rouge *= MASQUE;
        vert *= MASQUE;
        bleu *= MASQUE;

        return SDL_MapRGB(formatDest, rouge, vert, bleu);

}

int main(int argc, char** argv) {

        SDL_Init(SDL_INIT_VIDEO);

        Repere r = { NULL, -2, 1, -1, 1 };

        SDL_Surface* const ecran = SDL_SetVideoMode(
                LARGEUR_PX,
                HAUTEUR_PX,
                32,
                SDL_HWSURFACE | SDL_DOUBLEBUF
        );

        r.s = SDL_CreateRGBSurface(
                SDL_HWSURFACE,
                ecran->w, ecran->h,
                ecran->format->BitsPerPixel,
                ecran->format->Rmask,
                ecran->format->Gmask,
                ecran->format->Bmask,
                ecran->format->Amask
        );

        SDL_WM_SetCaption("Mandelbrot", NULL);

        enum {
                QUITTER,
                RIEN,
                AFFICHER,
                CALCULER,
                MOUVEMENT_OU_ZOOM,
                RECTANGLE_OU_DEZOOM
        } etat = CALCULER;

        int sx, sy, ex, ey;

        Repere r2 = r;

        SDL_Event event;
        while (etat != QUITTER && SDL_WaitEvent(&event)) {

                do {

                        switch (event.type) {

                                case SDL_KEYDOWN:
                                if (event.key.keysym.sym != SDLK_ESCAPE)
                                        break;
                                case SDL_QUIT:
                                etat = QUITTER;
                                break;

                                case SDL_MOUSEBUTTONDOWN:
                                sx = ex = event.button.x;
                                sy = ey = event.button.y;
                                if (event.button.button == SDL_BUTTON_LEFT)
                                        etat = MOUVEMENT_OU_ZOOM;
                                else if (event.button.button == SDL_BUTTON_RIGHT)
                                        etat = RECTANGLE_OU_DEZOOM;
                                break;

                                case SDL_MOUSEMOTION:
                                ex = event.motion.x;
                                ey = event.motion.y;
                                break;

                                case SDL_MOUSEBUTTONUP:
                                ex = event.button.x;
                                ey = event.button.y;
                                ecranVersRepere(&r, sx, sy, &r2.x1, &r2.y1);
                                ecranVersRepere(&r, ex, ey, &r2.x2, &r2.y2);
                                if (etat == MOUVEMENT_OU_ZOOM) {
                                        etat = CALCULER;
                                        const double mx = r2.x2 - r2.x1;
                                        const double my = r2.y2 - r2.y1;
                                        if (mx != 0 || my != 0) {
                                                r2 = r;
                                                r2.x1 -= mx;
                                                r2.x2 -= mx;
                                                r2.y1 -= my;
                                                r2.y2 -= my;
                                        } else {
                                                const double cx = r2.x1;
                                                const double cy = r2.y1;
                                                r2 = r;
                                                r2.x1 += cx;
                                                r2.x2 += cx;
                                                r2.y1 += cy;
                                                r2.y2 += cy;
                                                r2.x1 /= 2;
                                                r2.x2 /= 2;
                                                r2.y1 /= 2;
                                                r2.y2 /= 2;
                                        }
                                } else if (etat == RECTANGLE_OU_DEZOOM) {
                                        if (sx == ex && sy == ey) {
                                                const double cx = r2.x1;
                                                const double cy = r2.y1;
                                                r2 = r;
                                                r2.x1 *= 2;
                                                r2.x2 *= 2;
                                                r2.y1 *= 2;
                                                r2.y2 *= 2;
                                                r2.x1 -= cx;
                                                r2.x2 -= cx;
                                                r2.y1 -= cy;
                                                r2.y2 -= cy;
                                                etat = CALCULER;
                                        } else if (sx != ex && sy != ey)
                                                etat = CALCULER;
                                        else
                                                etat = AFFICHER;
                                }
                                r = r2;
                                break;

                                default:
                                break;

                        }

                } while (SDL_PollEvent(&event));

                if (etat == CALCULER) {

                        if (r.x1 > r.x2) {
                                const double tmp = r.x1;
                                r.x1 = r.x2;
                                r.x2 = tmp;
                        }

                        if (r.y1 > r.y2) {
                                const double tmp = r.y1;
                                r.y1 = r.y2;
                                r.y2 = tmp;
                        }

                        mandelbrot(&r);

                        etat = AFFICHER;

                }

                if (etat == AFFICHER || etat == RECTANGLE_OU_DEZOOM)
                        SDL_BlitSurface(r.s, NULL, ecran, NULL);
                else if (etat == MOUVEMENT_OU_ZOOM) {
                        SDL_FillRect(ecran, NULL, SDL_MapRGB(ecran->format, 0, 0, 0));
                        SDL_Rect dest = ecran->clip_rect;
                        dest.x += ex - sx;
                        dest.y += ey - sy;
                        SDL_BlitSurface(r.s, NULL, ecran, &dest);
                }

                if (etat == RECTANGLE_OU_DEZOOM && (sx != ex || sy != ey)) {

                        int x1 = (sx < ex ? sx : ex);
                        int x2 = (x1 == sx ? ex : sx);
                        int y1 = (sy < ey ? sy : ey);
                        int y2 = (y1 == sy ? ey : sy);

                        int x, y;
                        for (y = 0; y < r.s->h; ++y) {
                                for (x = 0; x < r.s->w; ++x) {

                                        Uint32* const p = pixel(ecran, x, y);

                                        if (x == x1 || x == x2 || y == y1 || y == y2)
                                                *p = surbrillance(&r, x, y, ecran->format);

                                        if (x < x1 || x > x2 || y < y1 || y > y2)
                                                *p = assombrir(&r, x, y, ecran->format);

                                }
                        }

                }

                if (etat == AFFICHER || etat == RECTANGLE_OU_DEZOOM || etat == MOUVEMENT_OU_ZOOM)
                        SDL_Flip(ecran);

                if (etat == AFFICHER)
                        etat = RIEN;

        }

        SDL_Quit();

        return 0;

}
