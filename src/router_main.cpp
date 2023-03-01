#include <algorithm>
#include <iostream>
#include <tuple>
#include "router.hpp"
void Router::main(){
    this->twoPinNetDecomposition();

    // sort Nets, try to add some heuristic
    // first pick the pins number, from the large to the small
    // then pick the bounding box, from the large to the small
    // std::sort(this->layout->netlist.begin(), this->layout->netlist.end(), [](const Net& a, const Net& b){
    //     if(a.pins.size() == b.pins.size()){
    //         int min_a_x = a.pins.at(0).x, max_a_x = a.pins.at(0).x;
    //         int min_b_x = b.pins.at(0).x, max_b_x = b.pins.at(0).x;
    //         for (const auto& pin : a.pins) {
    //             min_a_x = std::min(min_a_x, pin.x);
    //             max_a_x = std::max(max_a_x, pin.x);
    //         }
    //         for (const auto& pin : b.pins) {
    //             min_b_x = std::min(min_b_x, pin.x);
    //             max_b_x = std::max(max_b_x, pin.x);
    //         }

    //         int min_a_y = a.pins.at(0).y, max_a_y = a.pins.at(0).y;
    //         int min_b_y = b.pins.at(0).y, max_b_y = b.pins.at(0).y;
    //         for (const auto& pin : a.pins) {
    //             min_a_y = std::min(min_a_y, pin.x);
    //             max_a_y = std::max(max_a_y, pin.x);
    //         }
    //         for (const auto& pin : b.pins) {
    //             min_b_y = std::min(min_b_y, pin.x);
    //             max_b_y = std::max(max_b_y, pin.x);
    //         }

    //         return (max_a_x - min_a_x) * (max_a_y - min_a_y) < (max_b_x - min_b_x) * (max_b_y - min_b_y);
    //     }
    //     else return a.pins.size() > b.pins.size();
    // });
    Grid *pin_and_obstacle_grid = new Grid(this->layout);

    for(auto &n : layout->netlist){
        n.initTrees();
        for(auto &tpn : n.two_pins_net){
            std::vector<std::tuple<Net*, int, int>> rip_up_pair;
            if(!tree2tree_maze_routing(&n, n.tree->at(tpn.first), n.tree->at(tpn.second))){
                Edge tmp_edge = tree2tree_maze_routing(pin_and_obstacle_grid, &n, n.tree->at(tpn.first), n.tree->at(tpn.second));
                std::vector<Edge*> rip_up_candidate;
                std::unordered_set<Edge*> remove_duplicate;
                for(auto s : tmp_edge.segments){
                    if(s->z == 0){
                        for(int i = s->getX(); i <= s->getNeighbor(); i++){
                            for(auto c : grid->graph.at(i).at(s->getY()).at(s->z)->cur_edges){
                                if(remove_duplicate.count(c)) continue;
                                remove_duplicate.insert(c);
                                rip_up_candidate.push_back(c);
                            }
                        }
                    }
                    else if(s->z == 1){
                        for(int i = s->getY(); i <= s->getNeighbor(); i++){
                            for(auto c : grid->graph.at(s->getX()).at(i).at(s->z)->cur_edges){
                                if(remove_duplicate.count(c)) continue;
                                remove_duplicate.insert(c);
                                rip_up_candidate.push_back(c);
                            }
                        }
                    }
                }
                bool find_candidate = false;
                do{
                    find_candidate = false;
                    while(rip_up_candidate.size() > 0){
                        auto r = rip_up_candidate.front(); rip_up_candidate.erase(rip_up_candidate.begin());

                        for(auto s : r->segments){
                            std::cout << s->toString() << "\n";
                        }
                        std::cout << "\n";

                        auto &current_net = layout->netlist.at(grid->graph.at(r->start_pin.x).at(r->start_pin.y).at(r->start_pin.z)->obstacle);
                        const auto &[souce_index, sink_index] = ripUpEdges(grid, r, current_net.tree, rip_up_candidate);
                        rip_up_pair.push_back(std::make_tuple(&current_net, souce_index, sink_index));
                        find_candidate = true;
                        break;
                    }
                }while(find_candidate);

                // rip_up_pair.push_back(std::make_tuple(&n, tpn.first, tpn.second));
                rip_up_pair.insert(rip_up_pair.begin(), std::make_tuple(&n, tpn.first, tpn.second));
            }
            else if(!n.tree->mergeTree(tpn.first, tpn.second)) {
                throw std::runtime_error("Error: merge tree error");
            }
            for(auto rup : rip_up_pair){
                const auto &[current_net, souce_index, sink_index] = rup;
                // degbug
                std::cout << "Net#" << current_net->id << "\n";
                std::cout << "Source: ";
                for(auto p : current_net->tree->at(souce_index)->pinlist){
                    std::cout << p.toString() << ", ";
                }
                std::cout << "\n";
                std::cout << "Sink: ";
                for(auto p : current_net->tree->at(sink_index)->pinlist){
                    std::cout << p.toString() << ", ";
                }
                std::cout << "\n";
                continue;
                // degbug
                if(!tree2tree_maze_routing(current_net, current_net->tree->at(souce_index), current_net->tree->at(sink_index))){
                    throw std::runtime_error("Error: need more rip-up");
                }
                else if (!current_net->tree->mergeTree(souce_index, sink_index)) {
                    throw std::runtime_error("Error: merge tree error");
                }
            }
            
        }
    }
}