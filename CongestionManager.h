//
// Created by bassam on 28/11/2019.
//

#ifndef RDT_SERVER_CONGESTIONMANAGER_H
#define RDT_SERVER_CONGESTIONMANAGER_H


class CongestionManager {

public:
    enum CongestionState {
        SLOW_START,
        FAST_RECOVERY,
        CONGESTION_AVOIDANCE
    };

private:
    enum CongestionState state = SLOW_START;

};


#endif //RDT_SERVER_CONGESTIONMANAGER_H
