#!/usr/bin/env python

import sys
import rospy
from obstacles.srv import *

SERVICE_NAME = 'check_connection'


def check_connection_client(x1, y1, x2, y2):

    rospy.wait_for_service(SERVICE_NAME)
    try:
        get_if_valid = rospy.ServiceProxy(SERVICE_NAME, CheckConnection)
        resp = get_if_valid(x1, y1, x2, y2)
        return resp.is_valid
    except rospy.ServiceException, e:
        rospy.loginfo("Service call failed: %s" % e)


def usage():
    return "Expecting 4 arguments: two points x1 y1 x2 y2"


if __name__ == "__main__":
    if len(sys.argv) == 5:
        x1 = float(sys.argv[1])
        y1 = float(sys.argv[2])
        x2 = float(sys.argv[3])
        y2 = float(sys.argv[4])
    else:
        print usage()
        sys.exit(1)
    print "Requesting check on connection between: (%s %s) - (%s %s)" % (x1, y1, x2, y2)
    is_valid = check_connection_client(x1, y1, x2, y2)
    print "After checking connection: connection is valid? %s" % (is_valid)
