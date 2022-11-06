#include "RenderWindow.h"
#include "Color.h"

class Disk
{
public:
	Disk(double x = 0, double y = 0, double radius = 0, double hue = 0, double sat = 0, double val = 0, double alph = 1, double friction = 1, double interp = 0);
	void set_interp(double interp);
	void set_hue(double hue);
	void set_sat(double sat);
	void set_val(double val);
	void set_alph(double alph);
	void set_color(double hue, double sat, double val);
	void set_friction(double friction);
	void set_color(double hue, double sat, double val, double alph);
	void mod_color(double hue, double sat, double val);
	void mod_color(double hue, double sat, double val, double alph);
	void set_radius(double radius);
	void get_radius(double* radius);
	void mod_radius(double radius);
	void position(double x, double y);
	void get_position(double* x, double* y);
	void tick(double dt = 1);
	void animate();
	void walls(double left, double right, double top, double bottom, double repulsion, double dt = 1.0);
	void draw(RenderWindow* window, double left, double right, double top, double bottom);
	static double norm(Disk first, Disk second);
	static double distance(Disk first, Disk second);
	static void interact(Disk& first, Disk& second, double left, double right, double top, double bottom, double repulsion, double dt = 1);
	static void pull(Disk& guide, Disk& follower, double attraction, double dt = 1);
	static bool close(Disk& first, Disk& second, double left, double right, double top, double bottom);
	
	void constrain(double x, double y, double vx, double vy, double attraction, double push, double dt = 1);
	void radiate(double x, double y, double r, double attraction, double dt = 1);

private:
	double x, y, radius, targradius,
	hue, sat, val, alph,
	targhue, targsat, targval, targalph,
	vx, vy,
	friction, interp;
};