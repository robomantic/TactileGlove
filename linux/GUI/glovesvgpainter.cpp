#include "glovesvgpainter.h"

static const char lookup[NO_GLOVE_ELEMENTS][9] =
//	A(tip)	    B(left)	C(middle)  D(right)  E		F	   G	      H		 I
    {"","path3443","path3419","path3427","path3431","path3393",//1 Thumb
     "path3389","path3395","path3397","path3391","path3403","path3401","path3409","path3407","path3433",//2 Pointer
     "path3423","path3471","path3467","path3465","path3359","path3459","path3411","path3449",		//3 Middle finger
     "path3447","path3363","path3451","path3415","path3413","path3461","path3469","path3361",		//4 Ring finger
     "path3383","path3479","path3477","path3475","path3385","path3387","path3381","path3375","path3365",//5 Little finger
     "path3371","path3357","path3369","path3377",		//6 Heel of the Hand (line under the fingers) pad
     "path3367","path3429","path3481","path3457",		//7 Ball of the thumb pad
     "path3439","path3435","path3441","path3445","path3437",	//8 Ball of the Hand (middle) pad
     "path3419","path3425","path3421","path3431","path3433","path3429","path3423","path3427","path3437"};						//9 Side of the hand pad


void
GloveSvgPainter::init_glovedata ()
{
  int i;
  gd->data_mutex = (pthread_mutex_t*) malloc (sizeof (pthread_mutex_t));
  if (gd->data_mutex == NULL)
    {
      perror ("init_glovedata: malloc");
      exit (EXIT_FAILURE);
    }

  pthread_mutex_init (gd->data_mutex, NULL);
  for (i = 0; i < NO_GLOVE_ELEMENTS; i++)
    gd->data_array[i] = 0;

}

void
GloveSvgPainter::reset_glove_data()
{
    if (0 != pthread_mutex_lock(gd->data_mutex))
    {
        perror ("GloveSvgPainter::reset_glove_data():pthreat_mutex_lock");
        exit (EXIT_FAILURE);
    }
    for (int i = 0; i < NO_GLOVE_ELEMENTS; i++)
      gd->data_array[i] = 0;
    if (0 != pthread_mutex_unlock(gd->data_mutex))
    {
        perror ("GloveSvgPainter::reset_glove_data():pthreat_mutex_unlock");
        exit (EXIT_FAILURE);
    }
}

void
GloveSvgPainter::new_glove_data_available(unsigned short* glove_update)
{
    int i;

    if (0 != pthread_mutex_lock (gd->data_mutex))
    {
        perror ("GloveSvgPainter::new_glove_data_available: pthread_mutex_lock");
        exit (EXIT_FAILURE);
    }
    for (i=0; i < NO_GLOVE_ELEMENTS; i++)
    {
        gd->data_array[i] = glove_update[i];
    }
    if (0 != pthread_mutex_unlock (gd->data_mutex))
    {
        perror ("GloveSvgPainter::new_glove_data_available: pthread_mutex_unlock");
        exit (EXIT_FAILURE);
    }
    update();
}

void
GloveSvgPainter::generate_random_glovedata ()
{
  int i;

  long int rndnumber;

  if (0 != pthread_mutex_lock (gd->data_mutex))
    {
      perror ("generate_new_glovedata: pthread_mutex_lock");
      exit (EXIT_FAILURE);
    }

  for (i=0; i < NO_GLOVE_ELEMENTS; i++)
    {
      rndnumber = random();

      if (rndnumber < (RAND_MAX / 2)) /* only give new value 50% of time */
        gd->data_array[i] = rndnumber&0xFFF;
    }
  if (0 != pthread_mutex_unlock (gd->data_mutex))
    {
      perror ("generate_new_glovedata: pthread_mutex_unlock");
      exit (EXIT_FAILURE);
    }
}
GloveSvgPainter::GloveSvgPainter(QWidget *parent) :
    QWidget(parent)
{
    gd = (glovedata_t*) malloc (sizeof (glovedata_t));
    if (NULL == gd)
    {
        perror ("main(): malloc");
        exit (EXIT_FAILURE);
    }
    qDomDocPtr = new QDomDocument ("Sensorlayout");
    QFile file("Sensorlayout04.svg");
    if (!file.open(QIODevice::ReadOnly))
        throw std::exception();
    if (!qDomDocPtr->setContent(&file)) {
       file.close();
       throw std::exception();
    }
    file.close();
    init_glovedata ();
    qSvgRendererPtr = new QSvgRenderer (this);

    QDomElement docElem = qDomDocPtr->documentElement();

    /* Finding nodes for the elements */
    QDomNode groot = docElem.firstChildElement(QString ("g"));
    if (groot.isNull())
    {
        std::cerr << "Unexpectedly encountered no g node" << std::endl;
        return;
    }
    QDomNode glevel1 = groot.firstChildElement(QString("g"));
    if (glevel1.isNull())
    {
        std::cerr << "Could not find Node g level 1" << std::endl;
        return;
    }
    QDomNode pathlevel = glevel1.firstChildElement(QString("path"));
    if (pathlevel.isNull())
    {
        std::cerr << "Could not find path level " << std::endl;
        return;
    }
    for (; !pathlevel.isNull(); pathlevel = pathlevel.nextSiblingElement("path")) {
         QDomNamedNodeMap qmap = pathlevel.attributes();
         QDomNode nid = qmap.namedItem(QString("id"));
         if (nid.isNull())
             continue;

         QDomNode nstyle = qmap.namedItem(QString("style"));
         for (int i = 0; i < NO_GLOVE_ELEMENTS; i++)
         {
             if (QString(lookup[i]).compare(nid.nodeValue())==0)
                 qDomNodeArray[i] = nstyle;
         }
    }

}

void GloveSvgPainter::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing,false);
    update_svg();
    qSvgRendererPtr->load(qDomDocPtr->toByteArray());
    qSvgRendererPtr->render(&painter);
    emit ready_for_more();
}

void
GloveSvgPainter::update_svg()
{
    int i;

    if (0 != pthread_mutex_lock(gd->data_mutex))
    {
        perror ("GloveSvgPainter::update_svg: pthread_mutex_lock");
        exit (EXIT_FAILURE);
    }
    for (i=0; i < NO_GLOVE_ELEMENTS; i++)
    {
        char value[256];
        unsigned int temp = gd->data_array[i];
        unsigned int color;
        if (temp > 4095) temp = 4095;
        if (temp <= 1365)
        {
            color = 0x100*(((1000*temp / 5353) > 255)?255:(1000*temp / 5353));
        }
        else
        {
            if (temp <= 2730)
            {
                color = (1000*(temp-1365) / 5353)*0x10000 + 0xff00;
            }
            else
            {
                color = 0x100*(0xff - (1000*(temp-2730) / 5353)) + 0xff0000;
            }
        }
        snprintf (value,256,"fill:#%06x;stroke=none",color);
        //std::cout << "Replacing with" << value << "Color = " << color << "temp = " << temp << std::endl;
        qDomNodeArray[i].setNodeValue(QString(value));
    }

    if (0 != pthread_mutex_unlock (gd->data_mutex))
    {
        perror ("GloveSvgPainter::update_svg: pthread_mutex_unlock");
        exit (EXIT_FAILURE);
    }

}
