#!/usr/bin/env python
import rospy
import json
from std_msgs.msg import String

pub=''
keys=['Speech','Alarm','Door','Television', 'Silence']
signals=dict.fromkeys(keys, 0.0)
picked=dict.fromkeys(keys, 0.0)
detected=dict.fromkeys(keys, False)
detectThreshold=0.6
resetThreshold=0.3

def callback(data):
    global pub, keys, signals, picked
    # rospy.loginfo(rospy.get_caller_id() + "I heard %s", data.data)
    dat=json.loads(data.data)
    rospy.loginfo("hello world %s" % rospy.get_time())

    # hello_str = "hello world %s" % rospy.get_time()
    picked=dict.fromkeys(keys, 0.0)
    for _,v in enumerate(dat[0]):
        [key, prob]=v

        if(key in keys):
            # rospy.loginfo(key in keys)
            picked[key]=float(prob)

    # update 
    for _, v in enumerate(keys):
        # rospy.loginfo(signals[v])
        signals[v]=signals[v]*0.3+picked[v]*0.7

    # detect
    for _, v in enumerate(keys):
        if(signals[v]> detectThreshold and detected[v]==False):
            detected[v]=True
            rospy.loginfo('publish:'+v)
            pub.publish(v)
        if(detected[v]==True and signals[v]<resetThreshold):
            detected[v]=False
    
def signal_node():
    global pub
    rospy.init_node('signal_node', anonymous=True)
    rospy.loginfo('starting')
    # rate = rospy.Rate(10) # 10hz

    # while not rospy.is_shutdown():
    #     rate.sleep()

    # In ROS, nodes are uniquely named. If two nodes with the same
    # name are launched, the previous one is kicked off. The
    # anonymous=True flag means that rospy will choose a unique
    # name for our 'signal_node' node so that multiple signal_nodes can
    # run simultaneously.

    rospy.Subscriber("/audio", String, callback)
    pub = rospy.Publisher('/signal', String, queue_size=10)

    # spin() simply keeps python from exiting until this node is stopped
    rospy.spin()

if __name__ == '__main__':
    signal_node()

    # except rospy.ROSInterruptException:
    #     pass


# #!/usr/bin/env python
# # license removed for brevity
# import rospy
# from std_msgs.msg import String

# def talker():
#     rospy.init_node('talker', anonymous=True)
#     rate = rospy.Rate(10) # 10hz
#     while not rospy.is_shutdown():
#         hello_str = "hello world %s" % rospy.get_time()
#         rospy.loginfo(hello_str)
#         pub.publish(hello_str)
#         rate.sleep()

# if __name__ == '__main__':
#     try:
#         talker()
#     except rospy.ROSInterruptException:
#         pass