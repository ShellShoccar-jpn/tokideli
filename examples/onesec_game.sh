#!/bin/sh

cat <<-TITLE
	######################
	##   One-sec Game   ##
	######################

	We challenge you to see how accurate your sense of timing is!
	Try hitting a key 11 times at accurate 1-second intervals when ready.

TITLE

typeliner -n 11 |
linets    -3dZ  |
awk '
  NR==1{print $3,"Here you go!";}
  NR> 1{
    t1=$2-1; t2=(t1<0)?-t1:t1; t3=(t1<0)?t1:" " t1;
    if        (t2<0.04) {a++;print $3,"Excellent!",t3;
    } else if (t2<0.10) {b++;print $3,"Good!     ",t3;
    } else if (t2<0.20) {c++;print $3,"OK!       ",t3;
    } else              {d++;print $3,"BAD!      ",t3;
    }
  }
  NR>10{exit;}
  END{
    print "\n*** Your Score ***";
    printf("Excellent:%02d Good:%02d OK:%02d BAD:%02d\n",a,b,c,d);
  }'
