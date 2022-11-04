#include "Disk.h"
#include <cmath>

Disk::Disk(double x, double y, double radius, double hue, double sat, double val, double alph, double friction, double interp) :
	x(x), y(y), radius(radius), targradius(radius),
	hue(hue), sat(sat), val(val), alph(alph),
	targhue(hue), targsat(sat), targval(val), targalph(alph),
	vx(0), vy(0), friction(friction), interp(interp)
{
}

void Disk::set_interp(double interp)
{
	this->interp = interp;
}

void Disk::set_hue(double hue)
{
	this->hue = targhue = hue;
}

void Disk::set_sat(double sat)
{
	this->sat = targsat = sat;
}

void Disk::set_val(double val)
{
	this->val = targval = val;
}

void Disk::set_alph(double alph)
{
	this->alph = targalph = alph;
}

void Disk::set_color(double hue, double sat, double val)
{
	set_hue(hue);
	set_sat(sat);
	set_val(val);
}

void Disk::set_friction(double friction)
{
	this->friction = friction;
}

void Disk::set_color(double hue, double sat, double val, double alph)
{
	set_color(hue, sat, val);
	set_alph(alph);
}

void Disk::mod_color(double hue, double sat, double val)
{
	targhue = hue;
	targsat = sat;
	targval = val;
}

void Disk::mod_color(double hue, double sat, double val, double alph)
{
	mod_color(hue, sat, val);
	targalph = alph;
}

void Disk::set_radius(double radius)
{
	this->radius = targradius = radius;
}

void Disk::get_radius(double* radius)
{
	*radius = this->radius;
}

void Disk::mod_radius(double radius)
{
	targradius = radius;
}

void Disk::position(double x, double y)
{
	this->x = x;
	this->y = y;
}

void Disk::get_position(double* x, double* y)
{
	*x = this->x;
	*y = this->y;
}

void Disk::tick(double dt)
{
	x += vx * dt;
	y += vy * dt;

	vx *= pow(friction, dt);
	vy *= pow(friction, dt);
}

void Disk::animate()
{
	double hue_discrep = hue - targhue;
	if (abs(hue_discrep - 1) < abs(hue_discrep))
		hue_discrep -= 1;
	if (abs(hue_discrep + 1) < abs(hue_discrep))
		hue_discrep += 1;
	hue = interp * hue_discrep + targhue;
	sat = interp * sat + (1 - interp) * targsat;
	val = interp * val + (1 - interp) * targval;
	alph = interp * alph + (1 - interp) * targalph;

	radius = interp * radius + (1 - interp) * targradius;
}

void Disk::walls(double left, double right, double top, double bottom, double repulsion, double dt)
{
	double tempx, tempy;
	tempx = left + x * (right - left);
	tempy = top + y * (bottom - top);

	double bounce = repulsion * dt;

	if (tempx < left + radius)
	{
		vx += (radius / (right - left) - x) * bounce;
		// vx *= friction;
	}
	if (tempx > right - radius)
	{
		vx += ((right - left - radius) / (right - left) - x) * bounce;
		// vx *= friction;
	}
	if (tempy < top + radius)
	{
		vy += (radius / (bottom - top) - y) * bounce;
		// vy *= friction;
	}
	if (tempy > bottom - radius)
	{
		vy += ((bottom - top - radius) / (bottom - top) - y) * bounce;
		// vy *= friction;
	}
}

void Disk::draw(RenderWindow* window, double left, double right, double top, double bottom)
{
	Color color;
	color.hsva(hue, sat, val, alph);
	window->color(color);
	window->circle(left + x * (right - left), top + y * (bottom - top), radius);
}

double Disk::norm(Disk first, Disk second)
{
	double dx = (first.x - second.x);
	double dy = (first.y - second.y);
	return dx * dx + dy * dy;
}

double distance(Disk first, Disk second)
{
	return sqrt(Disk::norm(first, second));
}

void Disk::interact(Disk& first, Disk& second, double left, double right, double top, double bottom, double repulsion, double dt)
{
	double dx = first.x - second.x;
	double dy = first.y - second.y;
	double width = left - right;
	double height = top - bottom;
	double norm = sqrt(width * width * dx * dx + height * height * dy * dy);
	double equilibrium = first.radius + second.radius;
	double overlap = equilibrium - norm;
	// norm * (equilibrium - norm)

	double bounce = repulsion * dt;
	if (norm < equilibrium)
	{
		first.vx  +=  bounce * dx / norm * overlap;
		first.vy  +=  bounce * dy / norm * overlap;
		second.vx += -bounce * dx / norm * overlap;
		second.vy += -bounce * dy / norm * overlap;
	}
}

void Disk::pull(Disk& guide, Disk& follower, double attraction, double dt)
{
	double dx = guide.x - follower.x;
	double dy = guide.y - follower.y;

	follower.vx = 0; 
	follower.vy = 0; 
	follower.x += dt * attraction * dx;
	follower.y += dt * attraction * dy;
}

bool Disk::close(Disk& first, Disk& second, double left, double right, double top, double bottom)
{
	double dx = first.x - second.x;
	double dy = first.y - second.y;
	double width = left - right;
	double height = top - bottom;
	double norm = sqrt(width * width * dx * dx + height * height * dy * dy);
	double equilibrium = first.radius + second.radius;
	return norm < equilibrium;
}

void Disk::constrain(double x, double y, double vx, double vy, double attraction, double push, double dt)
{
	double dx = this->x - x;
	double dy = this->y - y;
	double dot = (dx * vx + dy * vy) / (vx * vx + vy * vy);

	double px = attraction * dt * (vx * dot - dx);
	double py = attraction * dt * (vy * dot - dy);

	this->x += push * px;
	this->vx += (1 - push) * px;
	this->y += push * py;
	this->vy += (1 - push) * py;
}

void Disk::radiate(double x, double y, double r, double attraction, double dt)
{
	double dx = this->x - x;
	double dy = this->y - y;
	double norm = sqrt(dx * dx + dy * dy);

	this->constrain(x + dx * r / norm, y + dy * r / norm, -dy, dx, attraction * norm, dt);
}