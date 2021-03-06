/* Copyright (c) 2010-2011, Panos Louridas, GRNET S.A.

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

   * Neither the name of GRNET S.A, nor the names of its contributors
   may be used to endorse or promote products derived from this
   software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include <math.h>
#include <string>
#include <cstring>
#include <limits>
#include <cstddef>

#include "table.h"

#include <omp.h>

void Table::reset() {
    num_outgoing.clear();
    rows.clear();
    nodes_to_idx.clear();
    idx_to_nodes.clear();
    pr.clear();
}

Table::Table(double a, double c, size_t i, bool t, bool n, string d)
    : trace(t),
      alpha(a),
      convergence(c),
      max_iterations(i),
      delim(d),
      numeric(n) {
}

void Table::reserve(size_t size) {
    num_outgoing.reserve(size);
    rows.reserve(size);
}

size_t Table::get_num_rows() {
    return rows.size();
}

void Table::set_num_rows(size_t num_rows) {
    num_outgoing.resize(num_rows);
    rows.resize(num_rows);
}

void Table::error(const char *p,const char *p2) {
    cerr << p <<  ' ' << p2 <<  '\n';
    exit(1);
}

double Table::get_alpha() {
    return alpha;
}

void Table::set_alpha(double a) {
    alpha = a;
}

unsigned long Table::get_max_iterations() {
    return max_iterations;
}

void Table::set_max_iterations(unsigned long i) {
    max_iterations = i;
}

double Table::get_convergence() {
    return convergence;
}

void Table::set_convergence(double c) {
    convergence = c;
}

vector<double>& Table::get_pagerank() {
    return pr;
}

string Table::get_node_name(size_t index) {
    if (numeric) {
        stringstream s;
        s << index;
        return s.str();
    } else {
        return idx_to_nodes[index];
    }
}

map<size_t, string>& Table::get_mapping() {
    return idx_to_nodes;
}

bool Table::get_trace() {
    return trace;
}

void Table::set_trace(bool t) {
    trace = t;
}

bool Table::get_numeric() {
    return numeric;
}

void Table::set_numeric(bool n) {
    numeric = n;
}

string Table::get_delim() {
    return delim;
}

void Table::set_delim(string d) {
    delim = d;
}

/*
 * From a blog post at: http://bit.ly/1QQ3hv
 */
void Table::trim(string &str) {

    size_t startpos = str.find_first_not_of(" \t");

    if (string::npos == startpos) {
        str = "";
    } else {
        str = str.substr(startpos, str.find_last_not_of(" \t") - startpos + 1);
    }
}

size_t Table::insert_mapping(const string &key) {

    size_t index = 0;
    map<string, size_t>::const_iterator i = nodes_to_idx.find(key);
    if (i != nodes_to_idx.end()) {
        index = i->second;
    } else {
        index = nodes_to_idx.size();
        nodes_to_idx.insert(pair<string, size_t>(key, index));
        idx_to_nodes.insert(pair<size_t, string>(index, key));;
    }

    return index;
}

int Table::read_file(const string &filename) {

    pair<map<string, size_t>::iterator, bool> ret;

    reset();

    istream *infile;

    if (filename.empty()) {
      infile = &cin;
    } else {
      infile = new ifstream(filename.c_str());
      if (!infile) {
          error("Cannot open file", filename.c_str());
      }
    }

    size_t delim_len = delim.length();
    size_t linenum = 0;
    string line; // current line
    while (getline(*infile, line)) {
        string from, to; // from and to fields
        size_t from_idx, to_idx; // indices of from and to nodes
        size_t pos = line.find(delim);
        if (pos != string::npos) {
            from = line.substr(0, pos);
            trim(from);
            if (!numeric) {
                from_idx = insert_mapping(from);
            } else {
                from_idx = strtol(from.c_str(), NULL, 10);
            }
            to = line.substr(pos + delim_len);
            trim(to);
            if (!numeric) {
                to_idx = insert_mapping(to);
            } else {
                to_idx = strtol(to.c_str(), NULL, 10);
            }
            add_arc(from_idx, to_idx);
        }

        linenum++;
        if (linenum && ((linenum % 100000) == 0)) {
            cerr << "read " << linenum << " lines, "
                 << rows.size() << " vertices" << endl;
        }

        from.clear();
        to.clear();
        line.clear();
    }

    cerr << "read " << linenum << " lines, "
         << rows.size() << " vertices" << endl;

    nodes_to_idx.clear();

    if (infile != &cin) {
        delete infile;
    }
    reserve(idx_to_nodes.size());

    return 0;
}

/*
 * Taken from: M. H. Austern, "Why You Shouldn't Use set - and What You Should
 * Use Instead", C++ Report 12:4, April 2000.
 */
template <class Vector, class T>
bool Table::insert_into_vector(Vector& v, const T& t) {
    typename Vector::iterator i = lower_bound(v.begin(), v.end(), t);
    if (i == v.end() || t < *i) {
        v.insert(i, t);
        return true;
    } else {
        return false;
    }
}

bool Table::add_arc(size_t from, size_t to) {

    bool ret = false;
    size_t max_dim = max(from, to);
    if (trace) {
        cout << "checking to add " << from << " => " << to << endl;
    }
    if (rows.size() <= max_dim) {
        max_dim = max_dim + 1;
        if (trace) {
            cout << "resizing rows from " << rows.size() << " to "
                 << max_dim << endl;
        }
        rows.resize(max_dim);
        if (num_outgoing.size() <= max_dim) {
            num_outgoing.resize(max_dim);
        }
    }

    // CHANGE: rows to hold outgoing nodes.
    ret = insert_into_vector(rows[from], to);

    if (ret) {
        num_outgoing[from]++;
        if (trace) {
            cout << "added " << from << " => " << to << endl;
        }
    }

    return ret;
}

void Table::pagerank() {

    vector<size_t>::iterator ci; // current incoming
    double diff = 1;
    size_t i;
    size_t ii;
    double sum_pr; // sum of current pagerank vector elements
    double dangling_pr; // sum of current pagerank vector elements for dangling
    			// nodes
    unsigned long num_iterations = 0;
    vector<double> old_pr;

    size_t num_rows = rows.size();

    if (num_rows == 0) {
        return;
    }

    pr.resize(num_rows);

    //Change, not done in loop, makes first run less efficient
    old_pr = pr;
    pr[0] = 1;

    //int num_dangling = 0;

    while (diff > convergence && num_iterations < max_iterations) {

        sum_pr = 0;
        dangling_pr = 0;
        //num_dangling = 0;

        int pr_size = pr.size();

        // CHANGE: Vectorisation.
	      #pragma omp for simd
        for (size_t k = 0; k < pr_size; k++) {
            double cpr = pr[k];
            sum_pr += cpr;
            if (num_outgoing[k] == 0) {
                dangling_pr += cpr;
            }
        }

            /* Normalize so that we start with sum equal to one */
            // CHANGE: Vectorisation.
            #pragma omp for simd
            for (i = 0; i < pr_size; i++) {
                old_pr[i] = pr[i];
		            pr[i] = 0;
            }

        /*
         * After normalisation the elements of the pagerank vector sum
         * to one
         */
        sum_pr = 1;

        /* An element of the A x I vector; all elements are identical */
        double one_Av = alpha * dangling_pr / num_rows;

        /* An element of the 1 x I vector; all elements are identical */
        double one_Iv = (1 - alpha) * sum_pr / num_rows;

        /* The difference to be checked for convergence */
        std::vector<size_t>::iterator k;
        double from_pr;
        double h_vv;
        std::ptrdiff_t ptr_diff;
        //std::map<size_t, double> buffer;
        int buffer_size = 0;
        int buff_max_size = 80;

        //int written = 0;

        // CHANGE: Parallelised the loop.
        // num_threads(4)
        diff = 0;
	     #pragma omp parallel private(buffer_size, k, h_vv, ptr_diff, from_pr) reduction(+:diff)  num_threads(4)
        {
        vector<size_t> to_buff(buff_max_size);
        vector<double> val_buff(buff_max_size);
          #pragma omp for schedule(dynamic, 20000)
          for (i = 0; i < num_rows; i++) {
            // CHANGE: Reversed algorithm
            from_pr = old_pr[i];
            ptr_diff = rows[i].size();

            for (k = rows[i].begin(); k < rows[i].end(); k++) {
              h_vv = from_pr / ptr_diff;
              to_buff[buffer_size] = *k;
              val_buff[buffer_size] = h_vv;
              buffer_size++;
            }

          #pragma ivdep
            if (buffer_size >= (buff_max_size - 20)) {
              for (int l = 0; l < buffer_size; l++) {
                  int first = to_buff[l];
                  double second = val_buff[l];
              #pragma omp atomic update
                pr[first] += second;
              }
              buffer_size = 0;
            }
          }

          // Cleanup buffers.
          #pragma ivdep
          for (int l = 0; l < buffer_size; l++) {
                  int first = to_buff[l];
                  double second = val_buff[l];
              #pragma omp atomic update
                pr[first] += second;
          }
              buffer_size = 0;


        //diff = 0;

        // CHANGE: Vectorisation.
//#pragma omp single
        #pragma omp for
#pragma ivdep
        for (i = 0; i < num_rows; i++) {
          pr[i] = pr[i] * alpha + one_Av + one_Iv;
          diff += fabs(pr[i] - old_pr[i]);
        }


        }

        num_iterations++;
#pragma omp barrier
    }
}

void Table::print_params(ostream& out) {
    out << "alpha = " << alpha << " convergence = " << convergence
        << " max_iterations = " << max_iterations
        << " numeric = " << numeric
        << " delimiter = '" << delim << "'" << endl;
}

void Table::print_table() {
    vector< vector<size_t> >::iterator cr;
    vector<size_t>::iterator cc; // current column

    size_t i = 0;
    for (cr = rows.begin(); cr != rows.end(); cr++) {
        cout << i << ":[ ";
        for (cc = cr->begin(); cc != cr->end(); cc++) {
            if (numeric) {
                cout << *cc << " ";
            } else {
                cout << idx_to_nodes[*cc] << " ";
            }
        }
        cout << "]" << endl;
        i++;
    }
}

void Table::print_outgoing() {
    vector<size_t>::iterator cn;

    cout << "[ ";
    for (cn = num_outgoing.begin(); cn != num_outgoing.end(); cn++) {
        cout << *cn << " ";
    }
    cout << "]" << endl;

}

void Table::print_pagerank() {

    vector<double>::iterator cr;
    double sum = 0;

    cout.precision(numeric_limits<double>::digits10);

    cout << "(" << pr.size() << ") " << "[ ";
    for (cr = pr.begin(); cr != pr.end(); cr++) {
        cout << *cr << " ";
        sum += *cr;
        cout << "s = " << sum << " ";
    }
    cout << "] "<< sum << endl;
}

void Table::print_pagerank_v() {

    size_t i;
    size_t num_rows = pr.size();
    double sum = 0;

    cout.precision(numeric_limits<double>::digits10);

    for (i = 0; i < num_rows; i++) {
        if (!numeric) {
            cout << idx_to_nodes[i] << " = " << pr[i] << endl;
        } else {
            cout << i << " = " << pr[i] << endl;
        }
        sum += pr[i];
    }
    cerr << "s = " << sum << " " << endl;
}
