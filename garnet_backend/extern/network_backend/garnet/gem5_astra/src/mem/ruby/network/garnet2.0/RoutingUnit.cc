/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Niket Agarwal
 *          Tushar Krishna
 */


#include "mem/ruby/network/garnet2.0/RoutingUnit.hh"

#include "base/cast.hh"
#include "base/logging.hh"
#include "mem/ruby/network/garnet2.0/InputUnit.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"
#include "mem/ruby/slicc_interface/Message.hh"

#include <iostream>
#include <algorithm>    // std::find
#include <vector>       // std::vector

RoutingUnit::RoutingUnit(Router *router)
{
    m_router = router;
    m_routing_table.clear();
    m_weight_table.clear();
    arbitrator=0;
}

void
RoutingUnit::addRoute(std::vector<NetDest>& routing_table_entry)
{
    if (routing_table_entry.size() > m_routing_table.size()) {
        m_routing_table.resize(routing_table_entry.size());
    }
    for (int v = 0; v < routing_table_entry.size(); v++) {
        m_routing_table[v].push_back(routing_table_entry[v]);
    }
}

void
RoutingUnit::addWeight(int link_weight)
{
    m_weight_table.push_back(link_weight);
}

bool
RoutingUnit::supportsVnet(int vnet, std::vector<int> sVnets)
{
    // If all vnets are supported, return true
    if (sVnets.size() == 1 && sVnets[0] == -1) {
        return true;
    }

    // Find the vnet in the vector, return true
    if (std::find(sVnets.begin(), sVnets.end(), vnet) != sVnets.end()) {
        return true;
    }

    // Not supported vnet
    return false;
}

/*
 * This is the default routing algorithm in garnet.
 * The routing table is populated during topology creation.
 * Routes can be biased via weight assignments in the topology file.
 * Correct weight assignments are critical to provide deadlock avoidance.
 */
int
RoutingUnit::lookupRoutingTable(int vnet, NetDest msg_destination)
{
    // First find all possible output link candidates
    // For ordered vnet, just choose the first
    // (to make sure different packets don't choose different routes)
    // For unordered vnet, randomly choose any of the links
    // To have a strict ordering between links, they should be given
    // different weights in the topology file

    int output_link = -1;
    int min_weight = INFINITE_;
    std::vector<int> output_link_candidates;
    int num_candidates = 0;

    // Identify the minimum weight among the candidate output links
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

        if (m_weight_table[link] <= min_weight)
            min_weight = m_weight_table[link];
        }
    }

    // Collect all candidate output links with this minimum weight
    for (int link = 0; link < m_routing_table[vnet].size(); link++) {
        if (msg_destination.intersectionIsNotEmpty(
            m_routing_table[vnet][link])) {

            if (m_weight_table[link] == min_weight) {
                num_candidates++;
                output_link_candidates.push_back(link);
            }
        }
    }

    if (output_link_candidates.size() == 0) {
        fatal("Fatal Error:: No Route exists from this Router.");
        exit(0);
    }

    // Randomly select any candidate output link
    int candidate = 0;
    if (!(m_router->get_net_ptr())->isVNetOrdered(vnet))
        candidate = rand() % num_candidates;

    output_link = output_link_candidates.at(candidate);
    return output_link;
}


void
RoutingUnit::addInDirection(PortDirection inport_dirn, int inport_idx)
{
    m_inports_dirn2idx[inport_dirn] = inport_idx;
    m_inports_idx2dirn[inport_idx]  = inport_dirn;
}

void
RoutingUnit::addOutDirection(PortDirection outport_dirn, int outport_idx)
{
    m_outports_dirn2idx[outport_dirn] = outport_idx;
    m_outports_idx2dirn[outport_idx]  = outport_dirn;
}


int
RoutingUnit::outportCompute(RouteInfo &route, int inport,
                            PortDirection inport_dirn)
{
    int outport = -1;

    if (route.dest_router == m_router->get_id()) {

        // Multiple NIs may be connected to this router,
        // all with output port direction = "Local"
        // Get exact outport id from table
        outport = lookupRoutingTable(route.vnet, route.net_dest);
        return outport;
    }

    // Routing Algorithm set in GarnetNetwork.py
    // Can be over-ridden from command line using --routing-algorithm = 1
    RoutingAlgorithm routing_algorithm =
        (RoutingAlgorithm) m_router->get_net_ptr()->getRoutingAlgorithm();
    parsed_faulty_links = m_router->get_net_ptr()->getFaultyLinks();

    switch (routing_algorithm) {
        case TABLE_:  outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
        case XY_:     outport =
            outportComputeXY(route, inport, inport_dirn); break;
        // any custom algorithm
        case CUSTOM_: outport =
            outportComputeCustom(route, inport, inport_dirn); break;
	    case RINGXY_: outport =
            outportComputeRINGXY(route, inport, inport_dirn); break;
        case ALLTOALL_: outport =
            outportComputeALLTOALL(route, inport, inport_dirn); break;
        case DORMIN_: outport =
            outportComputeDORMIN(route, inport, inport_dirn);break;
        case SANDWICH_: outport = 
            outportComputeSANDWICH(route, inport, inport_dirn);break;
        case SANDWICHES_: outport = 
            outportComputeSANDWICHES(route, inport, inport_dirn);break;
        default: outport =
            lookupRoutingTable(route.vnet, route.net_dest); break;
    }

    assert(outport != -1);
    return outport;
}

// XY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";

    int M5_VAR_USED num_rows = m_router->get_net_ptr()->getNumRows();
    int num_cols = m_router->get_net_ptr()->getNumCols();
    assert(num_rows > 0 && num_cols > 0);
    int my_id = m_router->get_id();
    int my_x = my_id % num_cols; 
    int my_y = my_id / num_cols;
    int dest_id = route.dest_router;
    int dest_x = dest_id % num_cols;
    int dest_y = dest_id / num_cols;
    int x_hops = abs(dest_x - my_x);
    int y_hops = abs(dest_y - my_y);
    bool x_dirn = (dest_x >= my_x);
    bool y_dirn = (dest_y >= my_y);
    assert(!(x_hops == 0 && y_hops == 0));

    if (x_hops > 0) {
        if (x_dirn) {
            assert(inport_dirn == "Local" || inport_dirn == "West");
            outport_dirn = "East";
        } else {
            assert(inport_dirn == "Local" || inport_dirn == "East");
            outport_dirn = "West";
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            // "Local" or "South" or "West" or "East"
            assert(inport_dirn != "North");
            outport_dirn = "North";
        } else {
            // "Local" or "North" or "West" or "East"
            assert(inport_dirn != "South");
            outport_dirn = "South";
        }
    } else {
        // x_hops == 0 and y_hops == 0
        // this is not possible
        // already checked that in outportCompute() function
        panic("x_hops == y_hops == 0");
    }
    return m_outports_dirn2idx[outport_dirn];
}

// RINGXY routing implemented using port directions
// Only for reference purpose in a Mesh
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeRINGXY(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
	std::vector<int>::iterator it;
    int my_vnet=route.vnet;
    //std::cout<<"heyyyyyyyyyyyyyy"<<std::endl;
    it=std::find(m_router->get_net_ptr()->local_vnets.begin(),m_router->get_net_ptr()->local_vnets.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->local_vnets.end()){
        outport_dirn="LocalEast"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->vertical_vnets1.begin(),m_router->get_net_ptr()->vertical_vnets1.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->vertical_vnets1.end()){
        outport_dirn="North"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->vertical_vnets2.begin(),m_router->get_net_ptr()->vertical_vnets2.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->vertical_vnets2.end()){
        outport_dirn="South"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->horizontal_vnets1.begin(),m_router->get_net_ptr()->horizontal_vnets1.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->horizontal_vnets1.end()){
        outport_dirn="East"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->horizontal_vnets2.begin(),m_router->get_net_ptr()->horizontal_vnets2.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->horizontal_vnets2.end()){
        outport_dirn="West"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->perpendicular_vnets1.begin(),m_router->get_net_ptr()->perpendicular_vnets1.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->perpendicular_vnets1.end()){
        outport_dirn="Zpositive"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->perpendicular_vnets2.begin(),m_router->get_net_ptr()->perpendicular_vnets2.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->perpendicular_vnets2.end()){
        outport_dirn="Znegative"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->fourth_vnets1.begin(),m_router->get_net_ptr()->fourth_vnets1.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->fourth_vnets1.end()){
        outport_dirn="Fpositive"+std::to_string(my_vnet);
    }
    it=std::find(m_router->get_net_ptr()->fourth_vnets2.begin(),m_router->get_net_ptr()->fourth_vnets2.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->fourth_vnets2.end()){
        outport_dirn="Fnegative"+std::to_string(my_vnet);
    }
    if(outport_dirn=="Unknown"){
        panic("Unknown Vnet!");
    }

    int local_num = m_router->get_net_ptr()->get_local_rings(); 
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
    int my_id = m_router->get_id();
    int my_x = my_id % local_num; 
    int my_y = (my_id % (local_num * horizontal_num)) / local_num;
    int my_z = my_id / (local_num * horizontal_num);
    int dst_id = route.dest_router;
    int dst_x = dst_id  % local_num;
    int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
    int dst_z = dst_id / (local_num * horizontal_num);
    int src_id = route.src_router;
    int src_x = src_id  % local_num;
    int src_y = (src_id % (local_num * horizontal_num)) / local_num;
    int src_z = src_id / (local_num * horizontal_num);

    return m_outports_dirn2idx[outport_dirn]; 
}
// AllTOALL routing implemented using port directions
// Only for reference purpose in a Switch-based Topology
// By default Garnet uses the routing table
int
RoutingUnit::outportComputeALLTOALL(RouteInfo route,
                              int inport,
                              PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
	std::vector<int>::iterator it;
    int my_vnet=route.vnet;
    it=std::find(m_router->get_net_ptr()->local_vnets.begin(),m_router->get_net_ptr()->local_vnets.end(),my_vnet);
    if(it!=m_router->get_net_ptr()->local_vnets.end()){
        outport_dirn="LocalEast"+std::to_string(my_vnet);
    }
    else{
        if(m_router->get_net_ptr()->isNVSwitch){
            int dest_id = route.dest_router;
            int my_id = m_router->get_id();
            int num_cpus=m_router->get_net_ptr()->num_cpus;
            int links_per_tile=m_router->get_net_ptr()->links_per_tile;
            if(my_id<num_cpus){
                outport_dirn="Out"+std::to_string(arbitrator%links_per_tile);
                arbitrator++;
                //std::cout<<"output drin: "<<outport_dirn<<std::endl;

            }
            else{
                outport_dirn="Out"+std::to_string(dest_id);
                //if(my_id==num_cpus && dest_id==0){
                   // std::cout<<"Routing unit) packet is going to: "<<outport_dirn<<" from router id: "<<m_router->get_id()<<" dest: "<<route.dest_router<<" in time: "<<curTick()<<"  from vnet: "<<route.vnet<<std::endl;
                //}
            }
        }
        else{
            outport_dirn="Out"+std::to_string(my_vnet);
        }
    }
    
    return m_outports_dirn2idx[outport_dirn];
}
int
RoutingUnit::outportComputeDORMIN(RouteInfo &route,
                              int inport,
                              PortDirection inport_dirn)
{   
    PortDirection outport_dirn = "Unknown";
	std::vector<int>::iterator it;
    int my_vnet=route.vnet;
    int local_num = m_router->get_net_ptr()->get_local_rings(); 
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();

    int my_id = m_router->get_id();
    int my_x = my_id % local_num; 
    int my_y = (my_id % (local_num * horizontal_num)) / local_num;
    int my_z = my_id / (local_num * horizontal_num);

    int dst_id = route.dest_router;
    int dst_x = dst_id  % local_num;
    int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
    int dst_z = dst_id / (local_num * horizontal_num);

    int src_id = route.src_router;
    int src_x = src_id  % local_num;
    int src_y = (src_id % (local_num * horizontal_num)) / local_num;
    int src_z = src_id / (local_num * horizontal_num);
    // Implement the algorithm
    int x_hops = abs(dst_x - my_x);
    int y_hops = abs(dst_y - my_y);
    int z_hops = abs(dst_z - my_z);
    // Determine the direction (tiebreaking case keeps the same with NSDI 2024 Google Paper)
    int x_diff1 = 0; 
    int x_diff2 = 0;
    int y_diff1 = 0;
    int y_diff2 = 0;
    int z_diff1 = 0;
    int z_diff2 = 0;
    if (dst_x >= src_x) {
        x_diff1 = dst_x - src_x;
        x_diff2 = local_num - (dst_x - src_x);
    } else{
        x_diff1 = local_num - (src_x - dst_x);
        x_diff2 = src_x - dst_x;
    }

    if (dst_y >= src_y) {
        y_diff1 = dst_y - src_y;
        y_diff2 = horizontal_num - (dst_y - src_y);
    } else{
        y_diff1 = horizontal_num - (src_y - dst_y);
        y_diff2 = src_y - dst_y;
    }

    if (dst_z >= src_z) {
        z_diff1 = dst_z - src_z;
        z_diff2 = vertical_num - (dst_z - src_z);
    } else{
        z_diff1 = vertical_num - (src_z - dst_z);
        z_diff2 = src_z - dst_z;
    }
    bool x_dirn = (x_diff1 > x_diff2); // if x_dirn = 1, use anti-clockwise direction; else, use clockwise direction
    bool y_dirn = (y_diff1 > y_diff2);
    bool z_dirn = (z_diff1 > z_diff2);
    if (x_diff1 == x_diff2) {
        if (src_x % 2 == 0) {
            x_dirn = 0;// if src coordinate of this dimension is even: positive direction    
        } else {
            x_dirn = 1;
        }
    }

    if (y_diff1 == y_diff2) {
        if (src_y % 2 == 0) {
            y_dirn = 0; 
        } else {
            y_dirn = 1;
        }
    }

    if (z_diff1 == z_diff2) {
        if (src_z % 2 == 0) {
            z_dirn = 0;
        } else {
            z_dirn = 1;
        }
    }
    // Determine the outport_dirn
    if (x_hops > 0) {
        if (x_dirn) {
            outport_dirn = "LocalWest" + std::to_string(my_vnet);
        } else {
            outport_dirn = "LocalEast" + std::to_string(my_vnet);
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            outport_dirn = "West" + std::to_string(my_vnet);
        } else {
            outport_dirn = "East" + std::to_string(my_vnet);
        }  
    } else if (z_hops > 0) {
        if (z_dirn) {
            outport_dirn = "South" + std::to_string(my_vnet);
        } else {
            outport_dirn = "North" + std::to_string(my_vnet);
        } 
    } else {
        std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
    }
    Cross_Dateline_Judge(my_id, route, outport_dirn);
    return m_outports_dirn2idx[outport_dirn]; 
}


// Template for implementing custom routing algorithm
// using port directions. (Example adaptive)
int
RoutingUnit::outportComputeCustom(RouteInfo route,
                                 int inport,
                                 PortDirection inport_dirn)
{
    panic("%s placeholder executed", __FUNCTION__);
}

std::vector<int>
RoutingUnit::Generate_Path_Nodes(int src_id, int dst_id)
{
    // Get network dimensions
    int local_num = m_router->get_net_ptr()->get_local_rings();          // X dimension size
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings(); // Y dimension size
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();     // Z dimension size

    // Compute source and destination coordinates
    int src_x = src_id % local_num;
    int src_y = (src_id % (local_num * horizontal_num)) / local_num;
    int src_z = src_id / (local_num * horizontal_num);

    int dst_x = dst_id % local_num;
    int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
    int dst_z = dst_id / (local_num * horizontal_num);

    // Initialize current position to source coordinates
    int curr_x = src_x;
    int curr_y = src_y;
    int curr_z = src_z;

    // Store the path nodes
    std::vector<int> path_nodes;
    path_nodes.push_back(src_id);

    // Loop until we reach the destination
    while (!(curr_x == dst_x && curr_y == dst_y && curr_z == dst_z))
    {
        // Compute differences in each dimension
        int x_diff_cw = (dst_x - curr_x + local_num) % local_num;         // Clockwise distance in X
        int x_diff_ccw = (curr_x - dst_x + local_num) % local_num;        // Counter-clockwise distance in X

        int y_diff_cw = (dst_y - curr_y + horizontal_num) % horizontal_num;  // Clockwise distance in Y
        int y_diff_ccw = (curr_y - dst_y + horizontal_num) % horizontal_num; // Counter-clockwise distance in Y

        int z_diff_cw = (dst_z - curr_z + vertical_num) % vertical_num;    // Clockwise distance in Z
        int z_diff_ccw = (curr_z - dst_z + vertical_num) % vertical_num;   // Counter-clockwise distance in Z

        if (curr_x != dst_x)
        {
            bool x_dirn; // 0 for clockwise, 1 for counter-clockwise
            if (x_diff_cw < x_diff_ccw)
            {
                x_dirn = 0; 
            }
            else if (x_diff_cw > x_diff_ccw)
            {
                x_dirn = 1; 
            }
            else 
            {
                x_dirn = (src_x % 2 == 0) ? 0 : 1;
            }

            if (x_dirn == 0)
                curr_x = (curr_x + 1) % local_num;
            else
                curr_x = (curr_x - 1 + local_num) % local_num;
        }
        // Determine direction in Y dimension if X is aligned
        else if (curr_y != dst_y)
        {
            bool y_dirn; 
            if (y_diff_cw < y_diff_ccw)
            {
                y_dirn = 0; 
            }
            else if (y_diff_cw > y_diff_ccw)
            {
                y_dirn = 1; 
            }
            else 
            {
                y_dirn = (src_y % 2 == 0) ? 0 : 1;
            }

            if (y_dirn == 0)
                curr_y = (curr_y + 1) % horizontal_num;
            else
                curr_y = (curr_y - 1 + horizontal_num) % horizontal_num;
        }
        // Determine direction in Z dimension if X and Y are aligned
        else if (curr_z != dst_z)
        {
            bool z_dirn; 
            if (z_diff_cw < z_diff_ccw)
            {
                z_dirn = 0; 
            }
            else if (z_diff_cw > z_diff_ccw)
            {
                z_dirn = 1; 
            }
            else 
            {
                z_dirn = (src_z % 2 == 0) ? 0 : 1;
            }

            if (z_dirn == 0)
                curr_z = (curr_z + 1) % vertical_num;
            else
                curr_z = (curr_z - 1 + vertical_num) % vertical_num;
        }

        int curr_id = curr_z * (local_num * horizontal_num) + curr_y * local_num + curr_x;
        path_nodes.push_back(curr_id);
    }
    return path_nodes;
}


// Check if the faulty link is in the path between src_id and dst_id
bool
RoutingUnit::In_Path_Judge(RouteInfo route, int src_id, int dst_id, int fault_id1, int fault_id2)
{
    std::vector<int> path_nodes = Generate_Path_Nodes(src_id, dst_id);
    
    for (size_t i = 0; i < path_nodes.size() - 1; ++i)
    {
        int node1 = path_nodes[i];
        int node2 = path_nodes[i + 1];

        if ((node1 == fault_id1 && node2 == fault_id2) ||
            (node1 == fault_id2 && node2 == fault_id1))
        {
            return true; 
        }
    }
    return false; 
}

// Get the direction of faulty link
int
RoutingUnit::GetFaultLinkDirection(int fault_id1, int fault_id2)
{
    // Get network dimensions
    int local_num = m_router->get_net_ptr()->get_local_rings();        // X dimension size
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings(); // Y dimension size
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();    // Z dimension size

    // Compute coordinates of the two nodes
    int x1 = fault_id1 % local_num;
    int y1 = (fault_id1 % (local_num * horizontal_num)) / local_num;
    int z1 = fault_id1 / (local_num * horizontal_num);

    int x2 = fault_id2 % local_num;
    int y2 = (fault_id2 % (local_num * horizontal_num)) / local_num;
    int z2 = fault_id2 / (local_num * horizontal_num);

    // Compute minimal distances in each dimension considering torus wrap-around
    int x_diff = std::min((x1 - x2 + local_num) % local_num, (x2 - x1 + local_num) % local_num);
    int y_diff = std::min((y1 - y2 + horizontal_num) % horizontal_num, (y2 - y1 + horizontal_num) % horizontal_num);
    int z_diff = std::min((z1 - z2 + vertical_num) % vertical_num, (z2 - z1 + vertical_num) % vertical_num);

    int total_diff = x_diff + y_diff + z_diff;

    assert(total_diff == 1 && "The two nodes are not directly connected via a single-hop link.");

    if (x_diff == 1 && y_diff == 0 && z_diff == 0)
    {
        return 1; // X dimension
    }
    else if (x_diff == 0 && y_diff == 1 && z_diff == 0)
    {
        return 2; // Y dimension
    }
    else if (x_diff == 0 && y_diff == 0 && z_diff == 1)
    {
        return 3; // Z dimension
    }
    else
    {
        assert(false && "Invalid state: nodes differ in more than one dimension.");
        return -1;
    }
}

// Get the intermediate node ID during DORMIN routing
int
RoutingUnit::GetIntermediateNodeId(int src_id, int dst_id, int dimension)
{
    // Get network dimensions
    int local_num = m_router->get_net_ptr()->get_local_rings();        // X dimension size
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings(); // Y dimension size

    // Compute source node coordinates
    int x_s = src_id % local_num;
    int y_s = (src_id % (local_num * horizontal_num)) / local_num;
    int z_s = src_id / (local_num * horizontal_num);

    // Compute destination node coordinates
    int x_d = dst_id % local_num;
    int y_d = (dst_id % (local_num * horizontal_num)) / local_num;

    int inter_x = x_s;
    int inter_y = y_s;
    int inter_z = z_s;

    // Update coordinates based on the dimension
    if (dimension == 1)
    {
        inter_x = x_d; 
        inter_y = y_s;
        inter_z = z_s;
    }
    else if (dimension == 2)
    {
        inter_x = x_d; 
        inter_y = y_d; 
        inter_z = z_s;
    }
    else
    {
        std::cerr << "Invalid dimension input. Please input 1 or 2." << std::endl;
        return -1; 
    }

    int inter_id = inter_z * (local_num * horizontal_num) + inter_y * local_num + inter_x;
    return inter_id;
}

int 
RoutingUnit::outportComputeSANDWICH(RouteInfo &route,
                         int inport,
                         PortDirection inport_dirn)
{
    PortDirection outport_dirn = "Unknown";
    int my_vnet=route.vnet;
    
    assert(parsed_faulty_links.size() == 1);
    assert(parsed_faulty_links[0].size() == 2);
    int fault_id1 = parsed_faulty_links[0][0];
    int fault_id2 = parsed_faulty_links[0][1];


    // Getting coordinate
    int local_num = m_router->get_net_ptr()->get_local_rings(); 
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();

    int my_id = m_router->get_id();
    int my_x = my_id % local_num; 
    int my_y = (my_id % (local_num * horizontal_num)) / local_num;
    int my_z = my_id / (local_num * horizontal_num);

    int dst_id = route.dest_router;
    int dst_x = dst_id  % local_num;
    int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
    int dst_z = dst_id / (local_num * horizontal_num);

    int src_id = route.src_router;
    int src_x = src_id  % local_num;
    int src_y = (src_id % (local_num * horizontal_num)) / local_num;
    int src_z = src_id / (local_num * horizontal_num);

    // Get number of dims that routing traverses
    int routing_dim = 0;
    // Compare X dimension
    if (src_x != dst_x)
        routing_dim++;
    // Compare Y dimension
    if (src_y != dst_y)
        routing_dim++;
    // Compare Z dimension
    if (src_z != dst_z)
        routing_dim++;

    // Implement the algorithm
    int x_hops = abs(dst_x - my_x);
    int y_hops = abs(dst_y - my_y);
    int z_hops = abs(dst_z - my_z);
    // Determine the direction (tiebreaking case keeps the same with NSDI 2024 Google Paper)
    int x_diff1 = 0; 
    int x_diff2 = 0;
    int y_diff1 = 0;
    int y_diff2 = 0;
    int z_diff1 = 0;
    int z_diff2 = 0;

    if (dst_x >= src_x) {
        x_diff1 = dst_x - src_x;
        x_diff2 = local_num - (dst_x - src_x);
    } else{
        x_diff1 = local_num - (src_x - dst_x);
        x_diff2 = src_x - dst_x;
    }

    if (dst_y >= src_y) {
        y_diff1 = dst_y - src_y;
        y_diff2 = horizontal_num - (dst_y - src_y);
    } else{
        y_diff1 = horizontal_num - (src_y - dst_y);
        y_diff2 = src_y - dst_y;
    }

    if (dst_z >= src_z) {
        z_diff1 = dst_z - src_z;
        z_diff2 = vertical_num - (dst_z - src_z);
    } else{
        z_diff1 = vertical_num - (src_z - dst_z);
        z_diff2 = src_z - dst_z;
    }
    bool x_dirn = (x_diff1 > x_diff2); 
    bool y_dirn = (y_diff1 > y_diff2);
    bool z_dirn = (z_diff1 > z_diff2);
    if (x_diff1 == x_diff2) {
        if (src_x % 2 == 0) {
            x_dirn = 0;
        } else {
            x_dirn = 1;
        }
    }

    if (y_diff1 == y_diff2) {
        if (src_y % 2 == 0) {
            y_dirn = 0;
        } else {
            y_dirn = 1;
        }
    }

    if (z_diff1 == z_diff2) {
        if (src_z % 2 == 0) {
            z_dirn = 0;
        } else {
            z_dirn = 1;
        }
    }

    // Determine the direction for communication in the second bread of sandwich law
    int y_diff3 = 0;
    int y_diff4 = 0;
    int z_diff3 = 0;
    int z_diff4 = 0;
    if (dst_y >= my_y) {
        y_diff3 = dst_y - my_y;
        y_diff4 = horizontal_num - (dst_y - my_y);
    } else{
        y_diff3 = horizontal_num - (my_y - dst_y);
        y_diff4 = my_y - dst_y;
    }

    if (dst_z >= my_z) {
        z_diff3 = dst_z - my_z;
        z_diff4 = vertical_num - (dst_z - my_z);
    } else{
        z_diff3 = vertical_num - (my_z - dst_z);
        z_diff4 = my_z - dst_z;
    }
    bool my_y_dirn = (y_diff3 > y_diff4);
    bool my_z_dirn = (z_diff3 > z_diff4);

    if (y_diff3 == y_diff4) {
        if (my_y % 2 == 0) {
            my_y_dirn = 0;   
        } else {
            my_y_dirn = 1;
        }
    }

    if (z_diff3 == z_diff4) {
        if (my_z % 2 == 0) {
            my_z_dirn = 0;  
        } else {
            my_z_dirn = 1;
        }
    }

    // Judge for WFR (Wild-First-Routing)
    int fault_link_dir = GetFaultLinkDirection(fault_id1, fault_id2);
    int inter1_id = GetIntermediateNodeId(src_id, dst_id, 1);

    if (In_Path_Judge(route, src_id, dst_id, fault_id1, fault_id2)) { 
        if (fault_link_dir == 1 && my_id == src_id && routing_dim == 3) {
            // yXYZ routing 
            if (y_hops > 0) { 
                if (y_dirn) {
                    outport_dirn = "West" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "East" + std::to_string(my_vnet);
                }
            } else { 
                if (src_y % 2 == 1) { 
                    outport_dirn = "West" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "East" + std::to_string(my_vnet);
                }
            }
        } 
        else if (fault_link_dir == 2 && my_id == inter1_id && routing_dim == 3) {
            // XzYZ routing
            if (z_hops > 0) { 
                if (z_dirn) {
                    outport_dirn = "South" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "North" + std::to_string(my_vnet);
                }
            } else { 
                if (src_z % 2 == 1) { 
                    outport_dirn = "South" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "North" + std::to_string(my_vnet);
                } 
            }
        }
        else if (routing_dim == 2 && src_z == dst_z) {
            if (fault_link_dir == 1 && my_id == src_id) {
                // yXY routing
                if (y_hops > 0) { 
                    if (y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }
                } else { 
                    if (src_y % 2 == 1) { 
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }
                }
            } else if (fault_link_dir == 2 && my_id == inter1_id) {
                // XzYZ routing
                if (z_hops > 0) { 
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    }
                } else { 
                    if (src_z % 2 == 1) { 
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                }
            } else if (fault_link_dir == 2 && my_x == dst_x && my_y == dst_y && my_z != dst_z) {
                // XzYZ routing -> the last Z stage
                if (z_hops > 0) { 
                    if (my_z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    }
                } else { 
                    if (my_z % 2 == 1) { 
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_dirn = "LocalWest" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "LocalEast" + std::to_string(my_vnet);
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 2 && src_y == dst_y) {
            if (fault_link_dir == 1 && my_id == src_id) {
                // zXZ routing
                if (z_hops > 0) { 
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    }
                } else { 
                    if (src_z % 2 == 1) { 
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                }
            } else if (fault_link_dir == 3 && my_x == dst_x && my_y == dst_y) {  
                // XZ routing -> the last stage
                if (z_hops > 0) {
                    if (z_dirn) {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_dirn = "LocalWest" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "LocalEast" + std::to_string(my_vnet);
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 2 && src_x == dst_x) {
            if (fault_link_dir == 2 && my_id == src_id) {
                // zYZ routing
                if (z_hops > 0) { 
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    }
                } else { 
                    if (src_z % 2 == 1) { 
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                }
            } else if (fault_link_dir == 3 && my_x == dst_x && my_y == dst_y) {  
                // YZ routing -> the last stage
                if (z_hops > 0) {
                    if (z_dirn) {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_dirn = "LocalWest" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "LocalEast" + std::to_string(my_vnet);
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 1 && src_x != dst_x) {
            if (fault_link_dir == 1 && my_id == src_id) {
                // yXY routing
                if (y_hops > 0) { 
                    if (y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }
                } else { 
                    if (src_y % 2 == 1) { 
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }
                }
            } else if (fault_link_dir == 1 && my_x == dst_x && my_y != dst_y) {
                // yXY routing -> the last y stage 
                if (y_hops > 0) { 
                    if (my_y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }
                } else { 
                    if (my_y % 2 == 1) { 
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_dirn = "LocalWest" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "LocalEast" + std::to_string(my_vnet);
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 1 && src_y != dst_y) {
            if (fault_link_dir == 2 && my_id == src_id) {
                // zYZ routing
                if (z_hops > 0) { 
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    }
                } else { 
                    if (src_z % 2 == 1) { 
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                }
            } else if (fault_link_dir == 2 && my_y == dst_y && my_z != dst_z) {
                // zYZ routing -> the last Z stage
                if (z_hops > 0) { 
                    if (my_z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    }
                } else { 
                    if (my_z % 2 == 1) { 
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_dirn = "LocalWest" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "LocalEast" + std::to_string(my_vnet);
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_dirn = "West" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "East" + std::to_string(my_vnet);
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_dirn = "South" + std::to_string(my_vnet);
                    } else {
                        outport_dirn = "North" + std::to_string(my_vnet);
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        // Dedicated plan for Z routing 
        else if (fault_link_dir == 3 && my_x == dst_x && my_y == dst_y) {
            if (z_hops > 0) {
                if (z_dirn) {
                    outport_dirn = "North" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "South" + std::to_string(my_vnet);
                } 
            }
        }
        else {
            // Normal DORMIN
            if (x_hops > 0) {
                if (x_dirn) {
                    outport_dirn = "LocalWest" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "LocalEast" + std::to_string(my_vnet);
                }
            } else if (y_hops > 0) {
                if (y_dirn) {
                    outport_dirn = "West" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "East" + std::to_string(my_vnet);
                }  
            } else if (z_hops > 0) {
                if (z_dirn) {
                    outport_dirn = "South" + std::to_string(my_vnet);
                } else {
                    outport_dirn = "North" + std::to_string(my_vnet);
                } 
            } else {
                std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
            }
        }
    } else {
        // Normal DORMIN
        if (x_hops > 0) {
            if (x_dirn) {
                outport_dirn = "LocalWest" + std::to_string(my_vnet);
            } else {
                outport_dirn = "LocalEast" + std::to_string(my_vnet);
            }
        } else if (y_hops > 0) {
            if (y_dirn) {
                outport_dirn = "West" + std::to_string(my_vnet);
            } else {
                outport_dirn = "East" + std::to_string(my_vnet);
            }  
        } else if (z_hops > 0) {
            if (z_dirn) {
                outport_dirn = "South" + std::to_string(my_vnet);
            } else {
                outport_dirn = "North" + std::to_string(my_vnet);
            } 
        } else {
            std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
        }
    }
    
    Cross_Dateline_Judge(my_id, route, outport_dirn);
    return m_outports_dirn2idx[outport_dirn]; 
}

// Current SANDWICHES implementation for arbitrary two faulty links 
int 
RoutingUnit::outportComputeSANDWICHES(RouteInfo &route,
                         int inport,
                         PortDirection inport_dirn)
{
PortDirection outport_dirn = "Unknown";
int my_vnet=route.vnet;
int output; 
assert(parsed_faulty_links.size() == 2);
assert(parsed_faulty_links[0].size() == 2);
assert(parsed_faulty_links[1].size() == 2);
int fault_id1 = parsed_faulty_links[0][0];
int fault_id2 = parsed_faulty_links[0][1];
int fault_id3 = parsed_faulty_links[1][0];
int fault_id4 = parsed_faulty_links[1][1];

// Getting coordinate
int local_num = m_router->get_net_ptr()->get_local_rings(); 
int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
int vertical_num = m_router->get_net_ptr()->get_vertical_rings();

int my_id = m_router->get_id();
int my_x = my_id % local_num; 
int my_y = (my_id % (local_num * horizontal_num)) / local_num;
int my_z = my_id / (local_num * horizontal_num);

int dst_id = route.dest_router;
int dst_x = dst_id  % local_num;
int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
int dst_z = dst_id / (local_num * horizontal_num);

int src_id = route.src_router;
int src_x = src_id  % local_num;
int src_y = (src_id % (local_num * horizontal_num)) / local_num;
int src_z = src_id / (local_num * horizontal_num);

// Implement the algorithm
int x_hops = abs(dst_x - my_x);
int y_hops = abs(dst_y - my_y);
int z_hops = abs(dst_z - my_z);
// Determine the direction (tiebreaking case keeps the same with NSDI 2024 Google Paper)
int x_diff1 = 0; 
int x_diff2 = 0;
int y_diff1 = 0;
int y_diff2 = 0;
int z_diff1 = 0;
int z_diff2 = 0;

if (dst_x >= src_x) {
    x_diff1 = dst_x - src_x;
    x_diff2 = local_num - (dst_x - src_x);
} else{
    x_diff1 = local_num - (src_x - dst_x);
    x_diff2 = src_x - dst_x;
}
if (dst_y >= src_y) {
    y_diff1 = dst_y - src_y;
    y_diff2 = horizontal_num - (dst_y - src_y);
} else{
    y_diff1 = horizontal_num - (src_y - dst_y);
    y_diff2 = src_y - dst_y;
}
if (dst_z >= src_z) {
    z_diff1 = dst_z - src_z;
    z_diff2 = vertical_num - (dst_z - src_z);
} else{
    z_diff1 = vertical_num - (src_z - dst_z);
    z_diff2 = src_z - dst_z;
}

bool x_dirn = (x_diff1 > x_diff2); // if x_dirn = 1, use anti-clockwise direction; else, use clockwise direction
bool y_dirn = (y_diff1 > y_diff2);
bool z_dirn = (z_diff1 > z_diff2);

if (x_diff1 == x_diff2) {
    if (src_x % 2 == 0) {
        x_dirn = 0; 
    } else {
        x_dirn = 1;
    }
}
if (y_diff1 == y_diff2) {
    if (src_y % 2 == 0) {
        y_dirn = 0;   
    } else {
        y_dirn = 1;
    }
}
if (z_diff1 == z_diff2) {
    if (src_z % 2 == 0) {
        z_dirn = 0;
    } else {
        z_dirn = 1;
    }
}

std::pair<int, int> DOR_next = nextHopDORMIN(route, inport, inport_dirn); 
int next_node_id = DOR_next.first;
int output_direction = DOR_next.second;
std::pair<int, int> First_fault_pair;
std::pair<int, int> SANDWICH_next;
bool in_path_failure = In_Path_Judge(route, my_id, dst_id, fault_id1, fault_id2) || In_Path_Judge(route, my_id, dst_id, fault_id3, fault_id4);
if (in_path_failure) {
    First_fault_pair = Get_First_FaultLink_Direction(my_id, dst_id, fault_id1, fault_id2, fault_id3, fault_id4);
    int fault_link_dir = First_fault_pair.first; // 1: LocalEast/LocalWest; 2: East/West; 3: North/South
    int former_or_latter = First_fault_pair.second; // 0: former, 1: latter
    if (former_or_latter == 0) {
        SANDWICH_next = nextHopSANDWICH(route, inport, inport_dirn, fault_id1, fault_id2);
    } else if (former_or_latter == 1) {
        SANDWICH_next = nextHopSANDWICH(route, inport, inport_dirn, fault_id3, fault_id4);
    } else {
        std::cout << "Error: former_or_latter is neither 0 nor 1!" << std::endl;
    }
    next_node_id = SANDWICH_next.first;
    output_direction = SANDWICH_next.second;

    bool next_hop_failure = In_Path_Judge(route, my_id, next_node_id, fault_id1, fault_id2) || In_Path_Judge(route, my_id, next_node_id, fault_id3, fault_id4);
    if (next_hop_failure) {
        std::pair<int, int> reverse_pair = ReverseNode(route, my_id, output_direction);
        next_node_id = reverse_pair.first;
        output_direction = reverse_pair.second;
    }

    if (output_direction == 0) {
        outport_dirn = "LocalWest" + std::to_string(my_vnet);
    } else if (output_direction == 1) {
        outport_dirn = "LocalEast" + std::to_string(my_vnet);
    } else if (output_direction == 2) {
        outport_dirn = "West" + std::to_string(my_vnet);
    } else if (output_direction == 3) {
        outport_dirn = "East" + std::to_string(my_vnet);
    } else if (output_direction == 4) {
        outport_dirn = "South" + std::to_string(my_vnet);
    } else if (output_direction == 5) {
        outport_dirn = "North" + std::to_string(my_vnet);
    } else {
        std::cout << "New_SANDWICHES: output direction is not valid!" << std::endl;
    }
    Cross_Dateline_Judge(my_id, route, outport_dirn);
    return m_outports_dirn2idx[outport_dirn]; 
} else {
    bool fault_flag = Ring_Has_Fault(my_id, next_node_id, fault_id1, fault_id2, fault_id3, fault_id4);
    std::pair<int, int> output_pair = outportComputeDORMIN_Myid (route, inport, inport_dirn);
    output = output_pair.first;
    int dirn_index = output_pair.second;
    int input_index = parseDirectionToIndex(inport_dirn);
    if (fault_flag && (dirn_index == input_index)){
        if (dirn_index == 1) {
            outport_dirn = "LocalWest" + std::to_string(my_vnet);
        } else if (dirn_index == 0) {
            outport_dirn = "LocalEast" + std::to_string(my_vnet);
        } else if (dirn_index == 3) {
            outport_dirn = "West" + std::to_string(my_vnet);
        } else if (dirn_index == 2) {
            outport_dirn = "East" + std::to_string(my_vnet);
        } else if (dirn_index == 5) {
            outport_dirn = "South" + std::to_string(my_vnet);
        } else if (dirn_index == 4) {
            outport_dirn = "North" + std::to_string(my_vnet);
        } else {
            std::cout << "Ring_Has_Fault branch: output direction is not valid!" << std::endl;
        }  
        output = m_outports_dirn2idx[outport_dirn];
    } else {
        if (output_direction == 0) {
            outport_dirn = "LocalWest" + std::to_string(my_vnet);
        } else if (output_direction == 1) {
            outport_dirn = "LocalEast" + std::to_string(my_vnet);
        } else if (output_direction == 2) {
            outport_dirn = "West" + std::to_string(my_vnet);
        } else if (output_direction == 3) {
            outport_dirn = "East" + std::to_string(my_vnet);
        } else if (output_direction == 4) {
            outport_dirn = "South" + std::to_string(my_vnet);
        } else if (output_direction == 5) {
            outport_dirn = "North" + std::to_string(my_vnet);
        } else {
            std::cout << "New_SANDWICHES: output direction is not valid!" << std::endl;
        }
    }
    Cross_Dateline_Judge(my_id, route, outport_dirn);
    return output; 
}

}

void
RoutingUnit::Cross_Dateline_Judge(int my_id, RouteInfo &route, PortDirection Dirn) 
{
// we set dataeline for wraparound links
int local_num = m_router->get_net_ptr()->get_local_rings(); 
int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
int vertical_num = m_router->get_net_ptr()->get_vertical_rings();

int my_x = my_id % local_num; 
int my_y = (my_id % (local_num * horizontal_num)) / local_num;
int my_z = my_id / (local_num * horizontal_num);
int last_routing_dim = route.last_routing_dim;

if (last_routing_dim == 1 &&
    !(Dirn.find("LocalEast") != std::string::npos ||
    Dirn.find("LocalWest") != std::string::npos)) {
    route.crossDateline = 0;
} else if (last_routing_dim == 2 &&
           !((Dirn.find("East") != std::string::npos &&
           Dirn.find("LocalEast") == std::string::npos) ||
           (Dirn.find("West") != std::string::npos &&
           Dirn.find("LocalWest") == std::string::npos))) {
    route.crossDateline = 0;
} else if (last_routing_dim == 3 &&
           !(Dirn.find("North") != std::string::npos ||
           Dirn.find("South") != std::string::npos)) {
    route.crossDateline = 0;
}


if (my_x == local_num-1 && Dirn.find("LocalEast") != std::string::npos) {
    route.crossDateline = 1;
} else if (my_y == horizontal_num-1 && Dirn.find("East") != std::string::npos && Dirn.find("LocalEast") == std::string::npos) {
    route.crossDateline = 1;
} else if (my_z == vertical_num-1 && Dirn.find("North") != std::string::npos) {
    route.crossDateline = 1;
}

if (my_x == 0 && Dirn.find("LocalWest") != std::string::npos) {
    route.crossDateline = 1;
} else if (my_y == 0 && Dirn.find("West") != std::string::npos && Dirn.find("LocalWest") == std::string::npos) {
    route.crossDateline = 1;
} else if (my_z == 0 && Dirn.find("South") != std::string::npos) {
    route.crossDateline = 1;
}

// set last_routing_dim at last
if (Dirn.find("LocalEast") != std::string::npos || Dirn.find("LocalWest") != std::string::npos) {
    route.last_routing_dim = 1;
} else if ((Dirn.find("East") != std::string::npos && Dirn.find("LocalEast") == std::string::npos) || (Dirn.find("West") != std::string::npos && Dirn.find("LocalWest") == std::string::npos)) {
    route.last_routing_dim = 2;
} else if (Dirn.find("North") != std::string::npos || Dirn.find("South") != std::string::npos) {
    route.last_routing_dim = 3;
}
}


// Galois change: return the `next node ID` and `outport direction` for DORMIN routing
std::pair<int, int>
RoutingUnit::nextHopDORMIN(RouteInfo &route,
                        int inport,
                        PortDirection inport_dirn)
{
// Galois change: we need to return:
// 1. next node ID
// 2. output direction: local = -1; localwest = 0; localeast = 1; west = 2; east = 3; south = 4; north = 5
int outport_direction;

int local_num = m_router->get_net_ptr()->get_local_rings();
int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
int vertical_num = m_router->get_net_ptr()->get_vertical_rings();

int my_id = m_router->get_id();
int my_x = my_id % local_num;
int my_y = (my_id % (local_num * horizontal_num)) / local_num;
int my_z = my_id / (local_num * horizontal_num);

int dst_id = route.dest_router;
int dst_x = dst_id % local_num;
int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
int dst_z = dst_id / (local_num * horizontal_num);

int src_id = route.src_router;
int src_x = src_id % local_num;
int src_y = (src_id % (local_num * horizontal_num)) / local_num;
int src_z = src_id / (local_num * horizontal_num);

int x_hops = abs(dst_x - my_x);
int y_hops = abs(dst_y - my_y);
int z_hops = abs(dst_z - my_z);

int x_diff1 = (dst_x >= src_x) ? dst_x - src_x : local_num - (src_x - dst_x);
int x_diff2 = (dst_x >= src_x) ? local_num - (dst_x - src_x) : src_x - dst_x;

int y_diff1 = (dst_y >= src_y) ? dst_y - src_y : horizontal_num - (src_y - dst_y);
int y_diff2 = (dst_y >= src_y) ? horizontal_num - (dst_y - src_y) : src_y - dst_y;

int z_diff1 = (dst_z >= src_z) ? dst_z - src_z : vertical_num - (src_z - dst_z);
int z_diff2 = (dst_z >= src_z) ? vertical_num - (dst_z - src_z) : src_z - dst_z;

bool x_dirn = (x_diff1 > x_diff2);
bool y_dirn = (y_diff1 > y_diff2);
bool z_dirn = (z_diff1 > z_diff2);

if (x_diff1 == x_diff2) x_dirn = (src_x % 2 != 0);
if (y_diff1 == y_diff2) y_dirn = (src_y % 2 != 0);
if (z_diff1 == z_diff2) z_dirn = (src_z % 2 != 0);

std::string outport_dirn;
int next_x = my_x, next_y = my_y, next_z = my_z;

if (x_hops > 0) {
    if (x_dirn) {
        next_x = (my_x - 1 + local_num) % local_num;
        outport_direction = 0;
    } else {
        next_x = (my_x + 1) % local_num;
        outport_direction = 1;
    }
} else if (y_hops > 0) {
    if (y_dirn) {
        next_y = (my_y - 1 + horizontal_num) % horizontal_num;
        outport_direction = 2;
    } else {
        next_y = (my_y + 1) % horizontal_num;
        outport_direction = 3;
    }
} else if (z_hops > 0) {
    if (z_dirn) {
        next_z = (my_z - 1 + vertical_num) % vertical_num;
        outport_direction = 4;
    } else {
        next_z = (my_z + 1) % vertical_num;
        outport_direction = 5;
    }
} else {
    return {my_id, -1};
}

int next_id = next_x + next_y * local_num + next_z * local_num * horizontal_num;
return {next_id, outport_direction};
}



// Galois change: return the `next node ID` and `outport direction` for SANDWICH routing
std::pair<int, int>
RoutingUnit::nextHopSANDWICH(RouteInfo &route,
                        int inport,
                        PortDirection inport_dirn,
                        int fault_id1,
                        int fault_id2) 
{
    // Galois change: we need to return:
    // 1. next node ID
    // 2. output direction: local = -1; localwest = 0; localeast = 1; west = 2; east = 3; south = 4; north = 5
    int outport_direction;

    PortDirection outport_dirn = "Unknown";
    int my_vnet=route.vnet;

    // Getting coordinate
    int local_num = m_router->get_net_ptr()->get_local_rings(); 
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();

    int my_id = m_router->get_id();
    int my_x = my_id % local_num; 
    int my_y = (my_id % (local_num * horizontal_num)) / local_num;
    int my_z = my_id / (local_num * horizontal_num);

    int dst_id = route.dest_router;
    int dst_x = dst_id  % local_num;
    int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
    int dst_z = dst_id / (local_num * horizontal_num);

    int src_id = route.src_router;
    int src_x = src_id  % local_num;
    int src_y = (src_id % (local_num * horizontal_num)) / local_num;
    int src_z = src_id / (local_num * horizontal_num);

    int routing_dim = 0;
    // Compare X dimension
    if (src_x != dst_x)
        routing_dim++;
    // Compare Y dimension
    if (src_y != dst_y)
        routing_dim++;
    // Compare Z dimension
    if (src_z != dst_z)
        routing_dim++;

    int x_hops = abs(dst_x - my_x);
    int y_hops = abs(dst_y - my_y);
    int z_hops = abs(dst_z - my_z);
    // Determine the direction (tiebreaking case keeps the same with NSDI 2024 Google Paper)
    int x_diff1 = 0; 
    int x_diff2 = 0;
    int y_diff1 = 0;
    int y_diff2 = 0;
    int z_diff1 = 0;
    int z_diff2 = 0;

    if (dst_x >= src_x) {
        x_diff1 = dst_x - src_x;
        x_diff2 = local_num - (dst_x - src_x);
    } else{
        x_diff1 = local_num - (src_x - dst_x);
        x_diff2 = src_x - dst_x;
    }

    if (dst_y >= src_y) {
        y_diff1 = dst_y - src_y;
        y_diff2 = horizontal_num - (dst_y - src_y);
    } else{
        y_diff1 = horizontal_num - (src_y - dst_y);
        y_diff2 = src_y - dst_y;
    }

    if (dst_z >= src_z) {
        z_diff1 = dst_z - src_z;
        z_diff2 = vertical_num - (dst_z - src_z);
    } else{
        z_diff1 = vertical_num - (src_z - dst_z);
        z_diff2 = src_z - dst_z;
    }
    bool x_dirn = (x_diff1 > x_diff2); 
    bool y_dirn = (y_diff1 > y_diff2);
    bool z_dirn = (z_diff1 > z_diff2);
    if (x_diff1 == x_diff2) {
        if (src_x % 2 == 0) {
            x_dirn = 0; 
        } else {
            x_dirn = 1;
        }
    }

    if (y_diff1 == y_diff2) {
        if (src_y % 2 == 0) {
            y_dirn = 0;  
        } else {
            y_dirn = 1;
        }
    }

    if (z_diff1 == z_diff2) {
        if (src_z % 2 == 0) {
            z_dirn = 0;   
        } else {
            z_dirn = 1;
        }
    }

    // Determine the direction for communication in the second bread of sandwich law
    int y_diff3 = 0;
    int y_diff4 = 0;
    int z_diff3 = 0;
    int z_diff4 = 0;

    if (dst_y >= my_y) {
        y_diff3 = dst_y - my_y;
        y_diff4 = horizontal_num - (dst_y - my_y);
    } else{
        y_diff3 = horizontal_num - (my_y - dst_y);
        y_diff4 = my_y - dst_y;
    }

    if (dst_z >= my_z) {
        z_diff3 = dst_z - my_z;
        z_diff4 = vertical_num - (dst_z - my_z);
    } else{
        z_diff3 = vertical_num - (my_z - dst_z);
        z_diff4 = my_z - dst_z;
    }
    bool my_y_dirn = (y_diff3 > y_diff4);
    bool my_z_dirn = (z_diff3 > z_diff4);

    if (y_diff3 == y_diff4) {
        if (my_y % 2 == 0) {
            my_y_dirn = 0; 
        } else {
            my_y_dirn = 1;
        }
    }

    if (z_diff3 == z_diff4) {
        if (my_z % 2 == 0) {
            my_z_dirn = 0;
        } else {
            my_z_dirn = 1;
        }
    }

    // Judge for WFR (Wild-First-Routing)
    int fault_link_dir = GetFaultLinkDirection(fault_id1, fault_id2);
    int inter1_id = GetIntermediateNodeId(src_id, dst_id, 1);

    if (In_Path_Judge(route, src_id, dst_id, fault_id1, fault_id2)) { 
        if (fault_link_dir == 1 && my_id == src_id && routing_dim == 3) {
            // yXYZ routing 
            outport_direction = 3;
        } 
        else if (fault_link_dir == 2 && my_id == inter1_id && routing_dim == 3) {
            // XzYZ routing
            outport_direction = 5;
        }
        else if (routing_dim == 2 && src_z == dst_z) {
            if (fault_link_dir == 1 && my_id == src_id) {
                // yXY routing
                outport_direction = 3;
            } else if (fault_link_dir == 2 && my_id == inter1_id) {
                // XzYZ routing
                outport_direction = 5;
            } else if (fault_link_dir == 2 && my_x == dst_x && my_y == dst_y && my_z != dst_z) {
                // XzYZ routing -> the last Z stage
                if (z_hops > 0) { 
                    if (my_z_dirn) {
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    }
                } else { 
                    if (my_z % 2 == 1) { 
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_direction = 0;
                    } else {
                        outport_direction = 1;
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_direction = 2;
                    } else {
                        outport_direction = 3;
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_direction = 4;
                    } else {
                        outport_direction = 4;
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 2 && src_y == dst_y) {
            if (fault_link_dir == 1 && my_id == src_id) {
                // zXZ routing
                outport_direction = 5;
            } else if (fault_link_dir == 3 && my_x == dst_x && my_y == dst_y) {  // Not perfect
                // XZ routing -> the last stage
                if (z_hops > 0) {
                    if (z_dirn) {
                        outport_direction = 5;
                    } else {
                        outport_direction = 4;
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_direction = 0;
                    } else {
                        outport_direction = 1;
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_direction = 2;
                    } else {
                        outport_direction = 3;
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 2 && src_x == dst_x) {
            if (fault_link_dir == 2 && my_id == src_id) {
                // zYZ routing
                outport_direction = 5;
            } else if (fault_link_dir == 3 && my_x == dst_x && my_y == dst_y) {  // Not perfect
                // YZ routing -> the last stage
                if (z_hops > 0) {
                    if (z_dirn) {
                        outport_direction = 5;
                    } else {
                        outport_direction = 4;
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_direction = 0;
                    } else {
                        outport_direction = 1;
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_direction = 2;
                    } else {
                        outport_direction = 3;
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 1 && src_x != dst_x) {
            if (fault_link_dir == 1 && my_id == src_id) {
                // yXY routing
                outport_direction = 3;
            } else if (fault_link_dir == 1 && my_x == dst_x && my_y != dst_y) {
                // yXY routing -> the last y stage 
                if (y_hops > 0) { 
                    if (my_y_dirn) {
                        outport_direction = 2;
                    } else {
                        outport_direction = 3;
                    }
                } else { 
                    if (my_y % 2 == 1) { 
                        outport_direction = 2;
                    } else {
                        outport_direction = 3;
                    }
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_direction = 0;
                    } else {
                        outport_direction = 1;
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_direction = 2;
                    } else {
                        outport_direction = 3;
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        else if (routing_dim == 1 && src_y != dst_y) {
            if (fault_link_dir == 2 && my_id == src_id) {
                // zYZ routing
                outport_direction = 5;
            } else if (fault_link_dir == 2 && my_y == dst_y && my_z != dst_z) {
                // zYZ routing -> the last Z stage
                if (z_hops > 0) { 
                    if (my_z_dirn) {
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    }
                } else { 
                    if (my_z % 2 == 1) { 
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    } 
                }
            } else {
                // Normal DORMIN
                if (x_hops > 0) {
                    if (x_dirn) {
                        outport_direction = 0;
                    } else {
                        outport_direction = 1;
                    }
                } else if (y_hops > 0) {
                    if (y_dirn) {
                        outport_direction = 2;
                    } else {
                        outport_direction = 3;
                    }  
                } else if (z_hops > 0) {
                    if (z_dirn) {
                        outport_direction = 4;
                    } else {
                        outport_direction = 5;
                    } 
                } else {
                    std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
                }
            }
        }
        // Dedicated plan for Z routing 
        else if (fault_link_dir == 3 && my_x == dst_x && my_y == dst_y) {
            if (z_hops > 0) {
                if (z_dirn) {
                    outport_direction = 5;
                } else {
                    outport_direction = 4;
                } 
            }
        }
        else {
            // Normal DORMIN
            if (x_hops > 0) {
                if (x_dirn) {
                    outport_direction = 0;
                } else {
                    outport_direction = 1;
                }
            } else if (y_hops > 0) {
                if (y_dirn) {
                    outport_direction = 2;
                } else {
                    outport_direction = 3;
                }  
            } else if (z_hops > 0) {
                if (z_dirn) {
                    outport_direction = 4;
                } else {
                    outport_direction = 5;
                } 
            } else {
                std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
            }
        }
    } else {
        // Normal DORMIN
        if (x_hops > 0) {
            if (x_dirn) {
                outport_direction = 0;
            } else {
                outport_direction = 1;
            }
        } else if (y_hops > 0) {
            if (y_dirn) {
                outport_direction = 2;
            } else {
                outport_direction = 3;
            }  
        } else if (z_hops > 0) {
            if (z_dirn) {
                outport_direction = 4;
            } else {
                outport_direction = 5;
            } 
        } else {
            outport_direction = -1;
            std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
        }
    }

    int next_x = my_x;
    int next_y = my_y;
    int next_z = my_z;
    switch (outport_direction) {
        case 0: // LocalWest
            next_x = (my_x - 1 + local_num) % local_num;
            break;
        case 1: // LocalEast
            next_x = (my_x + 1) % local_num;
            break;
        case 2: // West (Y-)
            next_y = (my_y - 1 + horizontal_num) % horizontal_num;
            break;
        case 3: // East (Y+)
            next_y = (my_y + 1) % horizontal_num;
            break;
        case 4: // South (Z-)
            next_z = (my_z - 1 + vertical_num) % vertical_num;
            break;
        case 5: // North (Z+)
            next_z = (my_z + 1) % vertical_num;
            break;
        default:
            std::cout << "SANDWICH: Stay at current node " << my_id << std::endl;
            return {my_id, -1};
    }

    int next_id = next_x + next_y * local_num + next_z * local_num * horizontal_num;
    return {next_id, outport_direction}; 
}


// Galois change: return the first `direction of faulty link` which on the path
std::pair<int, int>
RoutingUnit::Get_First_FaultLink_Direction(int src_id, int dst_id,
                                   int fault_id1_a, int fault_id1_b,
                                   int fault_id2_a, int fault_id2_b)
{
    // Galois change: return two values:
    // 1. The first direction of faulty link on the path (1/2/3)
    // 2. The former / latter faulty link it uses (0/1)
    std::vector<int> path_nodes = Generate_Path_Nodes(src_id, dst_id);

    int local = m_router->get_net_ptr()->get_local_rings();
    int horiz = m_router->get_net_ptr()->get_horizontal_rings();
    int z_stride = local * horiz;

    auto get_coord = [&](int id) -> std::tuple<int, int, int> {
        int x = id % local;
        int y = (id % (local * horiz)) / local;
        int z = id / (local * horiz);
        return {x, y, z};
    };

    auto get_dim = [&](int a, int b) -> int {
        auto [ax, ay, az] = get_coord(a);
        auto [bx, by, bz] = get_coord(b);
        if (ax != bx) return 1; // x
        if (ay != by) return 2; // y
        if (az != bz) return 3; // z
        return 0; // same node, invalid
    };

    auto is_match = [](int u1, int u2, int f1, int f2) -> bool {
        return (u1 == f1 && u2 == f2) || (u1 == f2 && u2 == f1);
    };

    for (size_t i = 0; i + 1 < path_nodes.size(); ++i) {
        int u = path_nodes[i];
        int v = path_nodes[i + 1];

        if (is_match(u, v, fault_id1_a, fault_id1_b)) {
            int dim = get_dim(u, v);
            return {dim, 0};  
        }
        if (is_match(u, v, fault_id2_a, fault_id2_b)) {
            int dim = get_dim(u, v);
            return {dim, 1}; 
        }
    }
    return {0, -1};
}

std::pair<int, int>
RoutingUnit::ReverseNode(RouteInfo &route,
                         int my_id,
                         int direction)
{
    // Galois change: we need to return:
    // 1. next node ID
    // 2. output direction: local = -1; localwest = 0; localeast = 1; west = 2; east = 3; south = 4; north = 5
    if (direction < 0 || direction > 5) {
        std::cerr << "Invalid direction before ReverseNode: " << direction << std::endl;
        panic("ReverseNode() input direction invalid.");
    }
    int local_num = m_router->get_net_ptr()->get_local_rings();
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();
    int my_x = my_id % local_num;
    int my_y = (my_id % (local_num * horizontal_num)) / local_num;
    int my_z = my_id / (local_num * horizontal_num);
    int next_x = my_x, next_y = my_y, next_z = my_z;
    int next_direct;

    if (direction == 0) {
        next_x = (my_x + 1) % local_num;
        next_direct = 1;
    } else if (direction == 1) {
        next_x = (my_x - 1 + local_num) % local_num;
        next_direct = 0;
    } else if (direction == 2) {
        next_y = (my_y + 1) % horizontal_num;
        next_direct = 3;
    } else if (direction == 3) {
        next_y = (my_y - 1 + horizontal_num) % horizontal_num;
        next_direct = 2;
    } else if (direction == 4) {
        next_z = (my_z + 1) % vertical_num;
        next_direct = 5;
    } else if (direction == 5) {
        next_z = (my_z - 1 + vertical_num) % vertical_num;
        next_direct = 4;
    } 
    int next_id = next_x + next_y * local_num + next_z * local_num * horizontal_num;
    return {next_id, next_direct};
}


// Routing for DORMIN (based on my_id instead of src_id)
std::pair<int, int>
RoutingUnit::outportComputeDORMIN_Myid(RouteInfo &route,
    int inport,
    PortDirection inport_dirn) 
{
    // Galois change: return two values:
    // 1: outport
    // 2. outport_dirn
    PortDirection outport_dirn = "Unknown";
	std::vector<int>::iterator it;
    int my_vnet=route.vnet;
    // Getting coordinate
    int dirn_index;
    int local_num = m_router->get_net_ptr()->get_local_rings(); 
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings();
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();

    int my_id = m_router->get_id();
    int my_x = my_id % local_num; 
    int my_y = (my_id % (local_num * horizontal_num)) / local_num;
    int my_z = my_id / (local_num * horizontal_num);

    int dst_id = route.dest_router;
    int dst_x = dst_id  % local_num;
    int dst_y = (dst_id % (local_num * horizontal_num)) / local_num;
    int dst_z = dst_id / (local_num * horizontal_num);

    int src_id = route.src_router;
    int src_x = src_id  % local_num;
    int src_y = (src_id % (local_num * horizontal_num)) / local_num;
    int src_z = src_id / (local_num * horizontal_num);

    int x_hops = abs(dst_x - my_x);
    int y_hops = abs(dst_y - my_y);
    int z_hops = abs(dst_z - my_z);
    // Determine the direction (tiebreaking case keeps the same with NSDI 2024 Google Paper)
    int x_diff1 = 0; 
    int x_diff2 = 0;
    int y_diff1 = 0;
    int y_diff2 = 0;
    int z_diff1 = 0;
    int z_diff2 = 0;
    if (dst_x >= my_x) {
        x_diff1 = dst_x - my_x;
        x_diff2 = local_num - (dst_x - my_x);
    } else{
        x_diff1 = local_num - (my_x - dst_x);
        x_diff2 = my_x - dst_x;
    }

    if (dst_y >= my_y) {
        y_diff1 = dst_y - my_y;
        y_diff2 = horizontal_num - (dst_y - my_y);
    } else{
        y_diff1 = horizontal_num - (my_y - dst_y);
        y_diff2 = my_y - dst_y;
    }

    if (dst_z >= my_z) {
        z_diff1 = dst_z - my_z;
        z_diff2 = vertical_num - (dst_z - my_z);
    } else{
        z_diff1 = vertical_num - (my_z - dst_z);
        z_diff2 = my_z - dst_z;
    }
    bool x_dirn = (x_diff1 > x_diff2); 
    bool y_dirn = (y_diff1 > y_diff2);
    bool z_dirn = (z_diff1 > z_diff2);
    if (x_diff1 == x_diff2) {
        if (src_x % 2 == 0) {
            x_dirn = 0;
        } else {
            x_dirn = 1;
        }
    }

    if (y_diff1 == y_diff2) {
        if (src_y % 2 == 0) {
            y_dirn = 0;    
        } else {
            y_dirn = 1;
        }
    }

    if (z_diff1 == z_diff2) {
        if (src_z % 2 == 0) {
            z_dirn = 0;   
        } else {
            z_dirn = 1;
        }
    }

    if (x_hops > 0) {
        if (x_dirn) {
            outport_dirn = "LocalWest" + std::to_string(my_vnet);
            dirn_index = 0;
        } else {
            outport_dirn = "LocalEast" + std::to_string(my_vnet);
            dirn_index = 1;
        }
    } else if (y_hops > 0) {
        if (y_dirn) {
            outport_dirn = "West" + std::to_string(my_vnet);
            dirn_index = 2;
        } else {
            outport_dirn = "East" + std::to_string(my_vnet);
            dirn_index = 3;
        }  
    } else if (z_hops > 0) {
        if (z_dirn) {
            outport_dirn = "South" + std::to_string(my_vnet);
            dirn_index = 4;
        } else {
            outport_dirn = "North" + std::to_string(my_vnet);
            dirn_index = 5;
        } 
    } else {
        std::cout << "DORMIN: Current Node is the destination!!!" << std::endl;
    }    

    return {m_outports_dirn2idx[outport_dirn], dirn_index}; 
}

// Galois change: parse the input direction and return it as a number
int 
RoutingUnit::parseDirectionToIndex(const PortDirection& dirn) {
    if (dirn.find("LocalWest") == 0) return 0;
    if (dirn.find("LocalEast") == 0) return 1;
    if (dirn.find("West") == 0) return 2;
    if (dirn.find("East") == 0) return 3;
    if (dirn.find("South") == 0) return 4;
    if (dirn.find("North") == 0) return 5;
    return -1; // unknown direction
}

// Galois change: judge whether the ring including the `given link` has link failure (currently only support two link failures)
bool
RoutingUnit::Ring_Has_Fault(int node1, int node2,
                             int fault_id1, int fault_id2,
                             int fault_id3, int fault_id4)
{
    // Get network dimensions
    int local_num = m_router->get_net_ptr()->get_local_rings();          // X
    int horizontal_num = m_router->get_net_ptr()->get_horizontal_rings(); // Y
    int vertical_num = m_router->get_net_ptr()->get_vertical_rings();     // Z

    // Get coordinates
    int x1 = node1 % local_num;
    int y1 = (node1 % (local_num * horizontal_num)) / local_num;
    int z1 = node1 / (local_num * horizontal_num);

    int x2 = node2 % local_num;
    int y2 = (node2 % (local_num * horizontal_num)) / local_num;
    int z2 = node2 / (local_num * horizontal_num);

    std::string dim;
    int ring_size;
    std::vector<int> ring_nodes;

    if (x1 != x2 && y1 == y2 && z1 == z2) {
        dim = "X";
        ring_size = local_num;
        for (int dx = 0; dx < local_num; ++dx) {
            int nid = z1 * (local_num * horizontal_num) + y1 * local_num + dx;
            ring_nodes.push_back(nid);
        }
    } else if (y1 != y2 && x1 == x2 && z1 == z2) {
        dim = "Y";
        ring_size = horizontal_num;
        for (int dy = 0; dy < horizontal_num; ++dy) {
            int nid = z1 * (local_num * horizontal_num) + dy * local_num + x1;
            ring_nodes.push_back(nid);
        }
    } else if (z1 != z2 && x1 == x2 && y1 == y2) {
        dim = "Z";
        ring_size = vertical_num;
        for (int dz = 0; dz < vertical_num; ++dz) {
            int nid = dz * (local_num * horizontal_num) + y1 * local_num + x1;
            ring_nodes.push_back(nid);
        }
    } else {
        std::cerr << "[ERROR] Ring_Has_Fault: node1 and node2 not in same dimension!" << std::endl;
        return false;
    }

    for (int i = 0; i < ring_size; ++i) {
        int curr = ring_nodes[i];
        int next = ring_nodes[(i + 1) % ring_size]; // wrap-around (torus)

        // Compare with both fault links (undirected match)
        if ((curr == fault_id1 && next == fault_id2) || (curr == fault_id2 && next == fault_id1) ||
            (curr == fault_id3 && next == fault_id4) || (curr == fault_id4 && next == fault_id3)) {
            return true;
        }
    }

    return false; 
}