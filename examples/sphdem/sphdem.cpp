/*
 * sphdem.cpp
 *
 *  Created on: 30 Jan 2014
 *      Author: mrobins
 */

#include "sphdem.h"
#include "Visualisation.h"

int main(int argc, char **argv) {

	auto sph = SphType::New();
	auto dem = DemType::New();
	auto params = ptr<Params>(new Params());

	const double pi = 3.14;
	const int n = 1000;
	const double L = 31.0/1000.0;
	const int ndem = 2;
	params->dem_diameter = 0.0011;
	params->dem_gamma = 0.0004;
	params->dem_k = 1.0e01;
	const double dem_vol = (1.0/6.0)*pi*pow(params->dem_diameter,3);
	const double dem_dens = 1160.0;
	params->dem_mass = dem_vol*dem_dens;
	const double dem_min_reduced_mass = 0.5*params->dem_mass;
	params->dem_dt = (1.0/50.0)*PI/sqrt(params->dem_k/dem_min_reduced_mass-pow(0.5*params->dem_gamma/dem_min_reduced_mass,2));


	auto geometry = [params](DemType::Value& i) {
		Vect3d acceleration;
		acceleration << 0,0,-9.8;
		const Vect3d& r = i.get_position();
		Vect3d& v = std::get<DEM_VELOCITY>(i.get_data());
		const double dem_diameter = std::get<PARAMS_DEM_DIAMETER>(*params);
		const double dem_k = std::get<PARAMS_DEM_K>(*params);
		const double dem_gamma = std::get<PARAMS_DEM_GAMMA>(*params);
		const double dem_mass = std::get<PARAMS_DEM_MASS>(*params);

		const double overlap = dem_diameter/2.0-r[2];
		if (overlap>0) {
			const double overlap_dot = -v[2];
			const Vect3d normal(0,0,1);
			const Vect3d gravity(0,0,-9.8);
			acceleration += (dem_k*overlap + dem_gamma*overlap_dot)*normal/dem_mass;
		}
		return acceleration;
	};

	int c = 0;
	dem->create_particles(ndem,[ndem,L,&c](DemType::Value& i) {
		Vect3d& v = std::get<DEM_VELOCITY>(i.get_data());
		Vect3d& f = std::get<DEM_FORCE>(i.get_data());

		v << 0,0,0;
		f << 0,0,0;
		Vect3d position(c*L/ndem,0,L);
		std::cout << "creating particle at "<<position<<std::endl;
		c++;
		return position;
	});

	Visualisation vis;
	auto grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	dem->copy_to_vtk_grid(grid);
	vis.glyph_points(grid);
	vis.start_render_loop();

	const Vect3d min(-2*dem_diameter,-2*dem_diameter,-2*dem_diameter);
	const Vect3d max(L+2*dem_diameter,L+2*dem_diameter,L+2*dem_diameter);
	dem->init_neighbour_search(min,max,dem_diameter);
	std::cout << "params are "<<*params<<std::endl;
	for (int i = 0; i < n; ++i) {
		std::cout <<"iteration "<<i<<std::endl;
		dem_start(dem,params,geometry);
		dem_end(dem,params,geometry);
		vis.stop_render_loop();
		dem->copy_to_vtk_grid(grid);
		vis.start_render_loop();
	}


}
