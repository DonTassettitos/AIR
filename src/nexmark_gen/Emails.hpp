/**
 * Copyright (c) 2020 University of Luxembourg. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF LUXEMBOURG AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE UNIVERSITY OF LUXEMBOURG OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

/*  
   NEXMark Generator -- Niagara Extension to XMark Data Generator

   Acknowledgements:
   The NEXMark Generator was developed using the xmlgen generator 
   from the XMark Benchmark project as a basis. The NEXMark
   generator generates streams of auction elements (bids, items
   for auctions, persons) as opposed to the auction files
   generated by xmlgen.  xmlgen was developed by Florian Waas.
   See http://www.xml-benchmark.org for information.

   Copyright (c) Dept. of  Computer Science & Engineering,
   OGI School of Science & Engineering, OHSU. All Rights Reserved.

   Permission to use, copy, modify, and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies
   of the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.

   THE AUTHORS AND THE DEPT. OF COMPUTER SCIENCE & ENGINEERING 
   AT OHSU ALLOW USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION, 
   AND THEY DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES 
   WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.

   This software was developed with support from NSF ITR award
   IIS0086002 and from DARPA through NAVY/SPAWAR 
   Contract No. N66001-99-1-8098.

*/

/*
 * Emails.hpp
 *
 *  Created on: April 10, 2020
 *      Author: damien.tassetti
 */

#pragma once

#include <vector>
#include <string>

namespace nexmark_gen {
using std::vector;
using std::string;

static const vector<string> email_extensions = {
    "ab.ca","ac.at","ac.be","ac.jp","ac.kr","ac.uk","acm.org",
    "airmail.net","arizona.edu","ask.com","att.com","auc.dk","auth.gr",
    "baylor.edu","bell-labs.com","bellatlantic.net","berkeley.edu",
    "brandeis.edu","broadquest.com","brown.edu","cabofalso.com","cas.cz",
    "clarkson.edu","clustra.com","cmu.edu","cnr.it","co.in","co.jp",
    "cohera.com","columbia.edu","compaq.com","computer.org",
    "concentric.net","conclusivestrategies.com","concordia.ca",
    "cornell.edu","crossgain.com","csufresno.edu","cti.gr","cwi.nl",
    "cwru.edu","dauphine.fr","dec.com","du.edu","duke.edu",
    "earthlink.net","edu.au","edu.cn","edu.hk","edu.sg","emc.com",
    "ernet.in","evergreen.edu","fernuni-hagen.de","filelmaker.com",
    "filemaker.com","forth.gr","forwiss.de","fsu.edu","gatech.edu",
    "gmu.edu","gte.com","hitachi.com","hp.com","ibm.com","imag.fr",
    "indiana.edu","infomix.com","informix.com","inria.fr","intersys.com",
    "itc.it","labs.com","lante.com","lbl.gov","lehner.net","llnl.gov",
    "lri.fr","lucent.com","memphis.edu","microsoft.com","mit.edu",
    "mitre.org","monmouth.edu","msn.com","msstate.edu","ncr.com",
    "neu.edu","newpaltz.edu","njit.edu","nodak.edu","ntua.gr","nwu.edu",
    "nyu.edu","ogi.edu","okcu.edu","oracle.com","ou.edu","panasonic.com",
    "pi.it","pitt.edu","poly.edu","poznan.pl","prc.com","propel.com",
    "purdue.edu","rice.edu","rpi.edu","rutgers.edu","rwth-aachen.de",
    "savera.com","sbphrd.com","sds.no","sdsc.edu","sfu.ca",
    "sleepycat.com","smu.edu","solidtech.com","stanford.edu","sun.com",
    "sunysb.edu","sybase.com","telcordia.com","temple.edu","toronto.edu",
    "tue.nl","twsu.edu","ualberta.ca","ubs.com","ucd.ie","ucdavis.edu",
    "ucf.edu","ucla.edu","ucr.edu","ucsb.edu","ucsd.edu","ufl.edu",
    "uga.edu","uic.edu","uiuc.edu","ul.pt","umass.edu","umb.edu",
    "umd.edu","umich.edu","umkc.edu","unbc.ca","unf.edu",
    "uni-freiburg.de","uni-mannheim.de","uni-marburg.de","uni-mb.si",
    "uni-muenchen.de","uni-muenster.de","uni-sb.de","uni-trier.de",
    "unical.it","unizh.ch","unl.edu","upenn.edu","uqam.ca","uregina.ca",
    "usa.net","ust.hk","uta.edu","utexas.edu","uu.se","uwaterloo.ca",
    "uwindsor.ca","uwo.ca","verity.com","versata.com","washington.edu",
    "whizbang.com","wisc.edu","wpi.edu","yahoo.com","yorku.ca",
    "zambeel.com"
};

static size_t extensions_maxlength = ([]{
    size_t res = 0;
    for (size_t i = 0; i < email_extensions.size(); i++)
    {
        if (email_extensions[i].length() > res){
            res = email_extensions[i].length();
        }
    }
    return res;
})();

};